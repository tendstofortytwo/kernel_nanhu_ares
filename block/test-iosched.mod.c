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
	{ 0x178ba18c, "_raw_spin_unlock" },
	{ 0x6643a0b4, "debugfs_create_dir" },
	{ 0x714d27c6, "bio_alloc" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x26ab68b5, "blk_put_request" },
	{ 0xf087137d, "__dynamic_pr_debug" },
	{ 0xfb0e29f, "init_timer_key" },
	{ 0xbc91d38d, "debugfs_remove_recursive" },
	{ 0x7d11c268, "jiffies" },
	{ 0xad8fe0a1, "__blk_run_queue" },
	{ 0xa7ecf324, "__init_waitqueue_head" },
	{ 0x3600854, "debugfs_create_u32" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xd5f2172f, "del_timer_sync" },
	{ 0x94545386, "init_request_from_bio" },
	{ 0x5f754e5a, "memset" },
	{ 0x27e1a049, "printk" },
	{ 0x45f903e7, "elv_unregister" },
	{ 0xbcee5082, "elv_dispatch_sort" },
	{ 0x8834396c, "mod_timer" },
	{ 0xfe53119b, "bio_put" },
	{ 0x8ea06178, "elv_register" },
	{ 0x6cc8db6d, "blk_rq_map_kern" },
	{ 0x542370a0, "kmem_cache_alloc" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0x1000e51, "schedule" },
	{ 0xc4097c34, "_raw_spin_lock" },
	{  0xf1338, "__wake_up" },
	{ 0x37a0cba, "kfree" },
	{ 0x9d669763, "memcpy" },
	{ 0x69ff5332, "prepare_to_wait" },
	{ 0xbc3d21af, "finish_wait" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x49ebacbd, "_clear_bit" },
	{ 0xf19382e, "blk_get_request" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

