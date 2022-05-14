// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2022 map220v <map220v300@gmail.com>
 * Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
 * Copyright (c) 2022, The Linux Foundation. All rights reserved.
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct s6e3fc3_ams667ym01 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset_gpio;
	bool prepared;
};

static inline
struct s6e3fc3_ams667ym01 *to_s6e3fc3_ams667ym01(struct drm_panel *panel)
{
	return container_of(panel, struct s6e3fc3_ams667ym01, panel);
}

#define dsi_generic_write_seq(dsi, seq...) do {				\
		static const u8 d[] = { seq };				\
		int ret;						\
		ret = mipi_dsi_generic_write(dsi, d, ARRAY_SIZE(d));	\
		if (ret < 0)						\
			return ret;					\
	} while (0)

static void s6e3fc3_ams667ym01_reset(struct s6e3fc3_ams667ym01 *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

static int s6e3fc3_ams667ym01_on(struct s6e3fc3_ams667ym01 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;
	
	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	msleep(30);

	dsi_generic_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	dsi_generic_write_seq(dsi, 0x9f, 0xa5, 0xa5);
	dsi_generic_write_seq(dsi, 0xf2,
			      0x00, 0x05, 0x0e, 0x58, 0x54, 0x00, 0x0c, 0x00,
			      0x04, 0x30, 0xb8, 0x30, 0xb8, 0x0c, 0x04, 0xbc,
			      0x26, 0xe8, 0x0c, 0x00, 0x04, 0x10, 0x00, 0x10,
			      0x26, 0xa8, 0x10, 0x00, 0x10, 0x10, 0x34, 0x10,
			      0x00, 0x40, 0x30, 0xc8, 0x00, 0xc8, 0x00, 0x00,
			      0xce);
	dsi_generic_write_seq(dsi, 0xf7, 0x0f);
	dsi_generic_write_seq(dsi, 0x35, 0x00);
	dsi_generic_write_seq(dsi, 0x2a, 0x00, 0x00, 0x04, 0x37);
	dsi_generic_write_seq(dsi, 0x2b, 0x00, 0x00, 0x09, 0x5f);
	dsi_generic_write_seq(dsi, 0xc2,
			      0x1b, 0x41, 0xb0, 0x0e, 0x00, 0x3c, 0x5a, 0x00,
			      0x00);
	dsi_generic_write_seq(dsi, 0xe5, 0x15);
	dsi_generic_write_seq(dsi, 0xed, 0x44, 0x4c, 0x20);
	dsi_generic_write_seq(dsi, 0xcc, 0x5c, 0x51);
	dsi_generic_write_seq(dsi, 0xb0, 0x00, 0x27, 0xf2);
	dsi_generic_write_seq(dsi, 0xf2, 0x00);
	dsi_generic_write_seq(dsi, 0xb0, 0x00, 0x92, 0x63);
	dsi_generic_write_seq(dsi, 0x63, 0x04);
	dsi_generic_write_seq(dsi, 0x60, 0x08, 0x00);
	dsi_generic_write_seq(dsi, 0xf7, 0x0f);
	dsi_generic_write_seq(dsi, 0x9f, 0x5a, 0x5a);
	dsi_generic_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	msleep(90);

	return 0;
}

static int s6e3fc3_ams667ym01_off(struct s6e3fc3_ams667ym01 *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;
	
	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	dsi_generic_write_seq(dsi, 0x9f, 0xa5, 0xa5);

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}
	msleep(20);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(120);

	dsi_generic_write_seq(dsi, 0x9f, 0x5a, 0x5a);

	return 0;
}

static int s6e3fc3_ams667ym01_prepare(struct drm_panel *panel)
{
	struct s6e3fc3_ams667ym01 *ctx = to_s6e3fc3_ams667ym01(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	if (ctx->prepared)
		return 0;

	//s6e3fc3_ams667ym01_reset(ctx);

	ret = s6e3fc3_ams667ym01_on(ctx);
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	ctx->prepared = true;
	return 0;
}

static int s6e3fc3_ams667ym01_unprepare(struct drm_panel *panel)
{
	struct s6e3fc3_ams667ym01 *ctx = to_s6e3fc3_ams667ym01(panel);
	//struct device *dev = &ctx->dsi->dev;
	//int ret;

	if (!ctx->prepared)
		return 0;

	/*ret = s6e3fc3_ams667ym01_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);*/

	ctx->prepared = false;
	return 0;
}

static const struct drm_display_mode s6e3fc3_ams667ym01_mode = {
	.clock = (1080 + 80 + 84 + 88) * (2400 + 15 + 2 + 2) * 90 / 1000,
	.hdisplay = 1080,
	.hsync_start = 1080 + 80,
	.hsync_end = 1080 + 80 + 84,
	.htotal = 1080 + 80 + 84 + 88,
	.vdisplay = 2400,
	.vsync_start = 2400 + 15,
	.vsync_end = 2400 + 15 + 2,
	.vtotal = 2400 + 15 + 2 + 2,
	.width_mm = 70,
	.height_mm = 155,
};

static int s6e3fc3_ams667ym01_get_modes(struct drm_panel *panel,
					    struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &s6e3fc3_ams667ym01_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs s6e3fc3_ams667ym01_panel_funcs = {
	.prepare = s6e3fc3_ams667ym01_prepare,
	.unprepare = s6e3fc3_ams667ym01_unprepare,
	.get_modes = s6e3fc3_ams667ym01_get_modes,
};

static int s6e3fc3_ams667ym01_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

// TODO: Check if /sys/class/backlight/.../actual_brightness actually returns
// correct values. If not, remove this function.
static int s6e3fc3_ams667ym01_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness;
}

static const struct backlight_ops s6e3fc3_ams667ym01_bl_ops = {
	.update_status = s6e3fc3_ams667ym01_bl_update_status,
	.get_brightness = s6e3fc3_ams667ym01_bl_get_brightness,
};

static struct backlight_device *
s6e3fc3_ams667ym01_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 486,
		.max_brightness = 486,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
					      &s6e3fc3_ams667ym01_bl_ops, &props);
}

static int s6e3fc3_ams667ym01_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct s6e3fc3_ams667ym01 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	/*ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");*/

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;

	drm_panel_init(&ctx->panel, dev, &s6e3fc3_ams667ym01_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->panel.backlight = s6e3fc3_ams667ym01_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "Failed to attach to DSI host: %d\n", ret);
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static int s6e3fc3_ams667ym01_remove(struct mipi_dsi_device *dsi)
{
	struct s6e3fc3_ams667ym01 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id s6e3fc3_ams667ym01_of_match[] = {
	{ .compatible = "samsung,s6e3fc3-ams667ym01" }, //Samsung Galaxy A72
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, s6e3fc3_ams667ym01_of_match);

static struct mipi_dsi_driver s6e3fc3_ams667ym01_driver = {
	.probe = s6e3fc3_ams667ym01_probe,
	.remove = s6e3fc3_ams667ym01_remove,
	.driver = {
		.name = "panel-s6e3fc3-ams667ym01",
		.of_match_table = s6e3fc3_ams667ym01_of_match,
	},
};
module_mipi_dsi_driver(s6e3fc3_ams667ym01_driver);

MODULE_AUTHOR("map220v <map220v300@gmail.com>");
MODULE_DESCRIPTION("MIPI-DSI based Panel Driver for AMS667YM01 AMOLED LCD with a S6E3FC3 controller");
MODULE_LICENSE("GPL v2");
