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
	{ 0xceee21f4, "kmalloc_caches" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x5934392b, "fb_register_client" },
	{ 0x6b116eb7, "dev_set_drvdata" },
	{ 0xf087137d, "__dynamic_pr_debug" },
	{ 0xf6ddf3b8, "mutex_unlock" },
	{ 0x91715312, "sprintf" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xe8220b2c, "device_register" },
	{ 0x9434e140, "__mutex_init" },
	{ 0x27e1a049, "printk" },
	{ 0x78e98764, "mutex_lock" },
	{ 0x542370a0, "kmem_cache_alloc" },
	{ 0x996bdb64, "_kstrtoul" },
	{ 0x37a0cba, "kfree" },
	{ 0x8e04eabb, "class_destroy" },
	{ 0xcc36f32e, "fb_unregister_client" },
	{ 0x204526c0, "device_unregister" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xe96077ca, "dev_set_name" },
	{ 0x7e1a21fc, "__class_create" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

