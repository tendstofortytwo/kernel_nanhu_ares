#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xdf8dcfc6, "module_layout" },
	{ 0x70d8225d, "sdio_writeb" },
	{ 0x84b00e81, "sdio_readb" },
	{ 0xf9a482f9, "msleep" },
	{ 0x98cc564a, "regulator_set_voltage" },
	{ 0xaa19a8b, "mmc_detect_change" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x6b116eb7, "dev_set_drvdata" },
	{ 0x1258d9d9, "regulator_disable" },
	{ 0xd2baffe5, "sdio_writesb" },
	{ 0x9fd0a1f9, "sdio_enable_func" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x486ba778, "sdio_claim_irq" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xc3b03b44, "crypto_ahash_digest" },
	{ 0x3441c3d6, "gpio_set_value_cansleep" },
	{ 0x9604ddff, "crypto_alloc_ablkcipher" },
	{ 0x65d6d0f0, "gpio_direction_input" },
	{ 0x27e1a049, "printk" },
	{ 0xad28b276, "regulator_bulk_get" },
	{ 0xa8f59416, "gpio_direction_output" },
	{ 0xfa818182, "sdio_readsb" },
	{ 0xed2f54f8, "sdio_unregister_driver" },
	{ 0xeb21f8d9, "sdio_set_host_pm_flags" },
	{ 0xd44dfe58, "sdio_release_irq" },
	{ 0x12b36d12, "crypto_destroy_tfm" },
	{ 0xd0e27aed, "sdio_memcpy_toio" },
	{ 0xfe990052, "gpio_free" },
	{ 0x201cb93f, "crypto_ahash_setkey" },
	{ 0x7a4497db, "kzfree" },
	{ 0xc3eafe2d, "pmapp_vreg_lpm_pincntrl_vote" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xa3570b67, "pmapp_clock_vote" },
	{ 0xbc5184dc, "sdio_register_driver" },
	{ 0x65880124, "sdio_memcpy_fromio" },
	{ 0x9800e09e, "sdio_claim_host" },
	{ 0x771fe94c, "dev_get_drvdata" },
	{ 0xd256f2e1, "sdio_set_block_size" },
	{ 0x47ac63ab, "sdio_disable_func" },
	{ 0x8a5c7a80, "regulator_enable" },
	{ 0xe954abb, "sdio_release_host" },
	{ 0x4a5e9e96, "crypto_alloc_ahash" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "559D7C7551546E5C5810ADF");
