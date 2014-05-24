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
	{ 0x12da5bb2, "__kmalloc" },
	{ 0xf9a482f9, "msleep" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x6b116eb7, "dev_set_drvdata" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0xc87c1f84, "ktime_get" },
	{ 0x942bd012, "usb_kill_urb" },
	{ 0x417e61fe, "__video_register_device" },
	{ 0xf6ddf3b8, "mutex_unlock" },
	{ 0x999e8297, "vfree" },
	{ 0xa7ecf324, "__init_waitqueue_head" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0xdd0a2ba2, "strlcat" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x5f754e5a, "memset" },
	{ 0x6a0b577c, "mutex_lock_interruptible" },
	{ 0x9434e140, "__mutex_init" },
	{ 0x27e1a049, "printk" },
	{ 0x346a92f7, "video_unregister_device" },
	{ 0x36c3d3ce, "usb_set_interface" },
	{ 0x73e20c1c, "strlcpy" },
	{ 0x78e98764, "mutex_lock" },
	{ 0x5cafc637, "usb_free_coherent" },
	{ 0x2196324, "__aeabi_idiv" },
	{ 0x9350463e, "vm_insert_page" },
	{ 0x18af7b53, "module_put" },
	{ 0x313a1a02, "usb_submit_urb" },
	{ 0x542370a0, "kmem_cache_alloc" },
	{ 0x7231e29d, "video_devdata" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0xbd7bf8d6, "input_register_device" },
	{ 0xd62c833f, "schedule_timeout" },
	{ 0x9df7f14c, "usb_clear_halt" },
	{ 0xd445c12a, "input_free_device" },
	{ 0xa0b04675, "vmalloc_32" },
	{  0xf1338, "__wake_up" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0x2920261f, "input_unregister_device" },
	{ 0x69ff5332, "prepare_to_wait" },
	{ 0xbc3d21af, "finish_wait" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x537c3334, "usb_ifnum_to_if" },
	{ 0xb81960ca, "snprintf" },
	{ 0x5a7fa645, "vmalloc_to_page" },
	{ 0xe75960ab, "usb_alloc_coherent" },
	{ 0x771fe94c, "dev_get_drvdata" },
	{ 0x5393e7a2, "usb_free_urb" },
	{ 0x33a7b33f, "video_ioctl2" },
	{ 0x797c2e4, "try_module_get" },
	{ 0x3f236a2c, "usb_alloc_urb" },
	{ 0x4cdb3178, "ns_to_timeval" },
	{ 0x4749a926, "input_allocate_device" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "FFBCF4BD2374601FE6CCFCE");
