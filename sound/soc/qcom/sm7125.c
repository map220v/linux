// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2020, Linaro Limited

#include <dt-bindings/sound/qcom,q6afe.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <linux/soundwire/sdw.h>
#include <sound/jack.h>
#include <linux/input-event-codes.h>
#include "qdsp6/q6afe.h"
#include "common.h"
#include "sdw.h"

#define DRIVER_NAME		"sm7125"
#define MI2S_BCLK_RATE		1536000

struct sm7125_snd_data {
	bool stream_prepared[AFE_PORT_MAX];
	struct snd_soc_card *card;
	u32 quin_mi2s_clk_count;
	struct sdw_stream_runtime *sruntime[AFE_PORT_MAX];
	struct snd_soc_jack jack;
	bool jack_setup;
};

static int sm7125_snd_init(struct snd_soc_pcm_runtime *rtd)
{
	struct sm7125_snd_data *data = snd_soc_card_get_drvdata(rtd->card);

	return qcom_snd_wcd_jack_setup(rtd, &data->jack, &data->jack_setup);
}

static int sm7125_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				     struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);
	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);
	struct snd_mask *fmt = hw_param_mask(params, SNDRV_PCM_HW_PARAM_FORMAT);

	rate->min = rate->max = 48000;
	channels->min = channels->max = 2;
	snd_mask_none(fmt);
	snd_mask_set_format(fmt, SNDRV_PCM_FORMAT_S16_LE);

	return 0;
}

static int sm7125_snd_startup(struct snd_pcm_substream *substream)
{
	unsigned int fmt = SND_SOC_DAIFMT_BP_FP;
	unsigned int codec_dai_fmt = SND_SOC_DAIFMT_BC_FC;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);
	struct snd_soc_dai *codec_dai = snd_soc_rtd_to_codec(rtd, 0);
	struct sm7125_snd_data *data = snd_soc_card_get_drvdata(rtd->card);

	switch (cpu_dai->id) {
	case QUINARY_MI2S_RX:
		codec_dai_fmt |= SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_I2S;
		if (++data->quin_mi2s_clk_count == 1) {
			snd_soc_dai_set_sysclk(cpu_dai,
				Q6AFE_LPASS_CLK_ID_QUI_MI2S_IBIT,
				MI2S_BCLK_RATE, SNDRV_PCM_STREAM_PLAYBACK);
		}
		snd_soc_dai_set_fmt(cpu_dai, fmt);
		snd_soc_dai_set_fmt(codec_dai, codec_dai_fmt);
		break;
	default:
		dev_err(rtd->dev, "%s: invalid dai id 0x%x\n", __func__,
			cpu_dai->id);
		break;
	}
	return 0;
}

static void sm7125_snd_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sm7125_snd_data *data = snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);

	switch (cpu_dai->id) {
	case QUINARY_MI2S_RX:
		if (--data->quin_mi2s_clk_count == 0) {
			snd_soc_dai_set_sysclk(cpu_dai,
							Q6AFE_LPASS_CLK_ID_QUI_MI2S_IBIT,
							0,
							SNDRV_PCM_STREAM_PLAYBACK);
		}
		break;
	default:
		dev_err(rtd->dev, "%s: invalid dai id 0x%x\n", __func__,
			cpu_dai->id);
		break;
	}
}

static int sm7125_snd_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai;
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);
	struct sm7125_snd_data *pdata = snd_soc_card_get_drvdata(rtd->card);
	struct sdw_stream_runtime *sruntime;
	int i, j;
	int ret;

	for_each_rtd_codec_dais(rtd, j, codec_dai) {
		if (!codec_dai->component->name_prefix)
			break;

		if (!strcmp(codec_dai->component->name_prefix, "Left")) {
			ret = snd_soc_dai_set_tdm_slot(
					codec_dai, 0x01,
					0x03, 8,
					16);
			if (ret < 0) {
				dev_err(rtd->dev,
					"DEV0 TDM slot err:%d\n", ret);
				return ret;
			}
		}

		if (!strcmp(codec_dai->component->name_prefix, "Right")) {
			ret = snd_soc_dai_set_tdm_slot(
					codec_dai, 0x02,
					0x03, 8,
					16);
			if (ret < 0) {
				dev_err(rtd->dev,
					"DEV1 TDM slot err:%d\n", ret);
				return ret;
			}
		}
	}

	switch (cpu_dai->id) {
	case WSA_CODEC_DMA_RX_0:
	case RX_CODEC_DMA_RX_0:
	case RX_CODEC_DMA_RX_1:
	case TX_CODEC_DMA_TX_0:
	case TX_CODEC_DMA_TX_1:
	case TX_CODEC_DMA_TX_2:
	case TX_CODEC_DMA_TX_3:
		for_each_rtd_codec_dais(rtd, i, codec_dai) {
			sruntime = snd_soc_dai_get_stream(codec_dai,
								substream->stream);
			if (sruntime != ERR_PTR(-EOPNOTSUPP))
				pdata->sruntime[cpu_dai->id] = sruntime;
		}
		break;
	}

	return 0;
}

static int sm7125_snd_wsa_dma_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);
	struct sm7125_snd_data *data = snd_soc_card_get_drvdata(rtd->card);
	struct sdw_stream_runtime *sruntime = data->sruntime[cpu_dai->id];
	int ret;

	if (!sruntime)
		return 0;

	if (data->stream_prepared[cpu_dai->id]) {
		sdw_disable_stream(sruntime);
		sdw_deprepare_stream(sruntime);
		data->stream_prepared[cpu_dai->id] = false;
	}

	ret = sdw_prepare_stream(sruntime);
	if (ret)
		return ret;

	/**
	 * NOTE: there is a strict hw requirement about the ordering of port
	 * enables and actual WSA881x PA enable. PA enable should only happen
	 * after soundwire ports are enabled if not DC on the line is
	 * accumulated resulting in Click/Pop Noise
	 * PA enable/mute are handled as part of codec DAPM and digital mute.
	 */

	ret = sdw_enable_stream(sruntime);
	if (ret) {
		sdw_deprepare_stream(sruntime);
		return ret;
	}
	data->stream_prepared[cpu_dai->id]  = true;

	return ret;
}

static int sm7125_snd_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);

	switch (cpu_dai->id) {
	case WSA_CODEC_DMA_RX_0:
	case WSA_CODEC_DMA_RX_1:
	case RX_CODEC_DMA_RX_0:
	case RX_CODEC_DMA_RX_1:
	case TX_CODEC_DMA_TX_0:
	case TX_CODEC_DMA_TX_1:
	case TX_CODEC_DMA_TX_2:
	case TX_CODEC_DMA_TX_3:
		return sm7125_snd_wsa_dma_prepare(substream);
	default:
		break;
	}

	return 0;
}

static int sm7125_snd_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sm7125_snd_data *data = snd_soc_card_get_drvdata(rtd->card);
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);
	struct sdw_stream_runtime *sruntime = data->sruntime[cpu_dai->id];

	switch (cpu_dai->id) {
	case WSA_CODEC_DMA_RX_0:
	case WSA_CODEC_DMA_RX_1:
	case RX_CODEC_DMA_RX_0:
	case RX_CODEC_DMA_RX_1:
	case TX_CODEC_DMA_TX_0:
	case TX_CODEC_DMA_TX_1:
	case TX_CODEC_DMA_TX_2:
	case TX_CODEC_DMA_TX_3:
		if (sruntime && data->stream_prepared[cpu_dai->id]) {
			sdw_disable_stream(sruntime);
			sdw_deprepare_stream(sruntime);
			data->stream_prepared[cpu_dai->id] = false;
		}
		break;
	default:
		break;
	}

	return 0;
}

static const struct snd_soc_ops sm7125_be_ops = {
	.startup = sm7125_snd_startup,
	.shutdown = sm7125_snd_shutdown,
	.hw_params = sm7125_snd_hw_params,
	.hw_free = sm7125_snd_hw_free,
	.prepare = sm7125_snd_prepare,
};

static void sm7125_add_be_ops(struct snd_soc_card *card)
{
	struct snd_soc_dai_link *link;
	int i;

	for_each_card_prelinks(card, i, link) {
		if (link->no_pcm == 1) {
			link->init = sm7125_snd_init;
			link->be_hw_params_fixup = sm7125_be_hw_params_fixup;
			link->ops = &sm7125_be_ops;
		}
	}
}

static int sm7125_platform_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card;
	struct sm7125_snd_data *data;
	struct device *dev = &pdev->dev;
	int ret;

	card = devm_kzalloc(dev, sizeof(*card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;

	card->owner = THIS_MODULE;
	/* Allocate the private data */
	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	card->dev = dev;
	dev_set_drvdata(dev, card);
	snd_soc_card_set_drvdata(card, data);
	ret = qcom_snd_parse_of(card);
	if (ret)
		return ret;

	card->driver_name = DRIVER_NAME;
	sm7125_add_be_ops(card);
	return devm_snd_soc_register_card(dev, card);
}

static const struct of_device_id snd_sm7125_dt_match[] = {
	{.compatible = "qcom,sm7125-sndcard"},
	{}
};

MODULE_DEVICE_TABLE(of, snd_sm7125_dt_match);

static struct platform_driver snd_sm7125_driver = {
	.probe  = sm7125_platform_probe,
	.driver = {
		.name = "snd-sm7125",
		.of_match_table = snd_sm7125_dt_match,
	},
};
module_platform_driver(snd_sm7125_driver);
MODULE_AUTHOR("map220v <map220v300@gmail.com>");
MODULE_DESCRIPTION("SM7125 ASoC Machine Driver");
MODULE_LICENSE("GPL v2");
