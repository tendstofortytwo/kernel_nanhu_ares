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
	{ 0x795a92f1, "single_release" },
	{ 0x49d9676b, "seq_read" },
	{ 0xf6068f42, "seq_lseek" },
	{ 0xc395779b, "mmc_unregister_driver" },
	{ 0x7fb55762, "mmc_register_driver" },
	{ 0x1424f59, "sg_copy_to_buffer" },
	{ 0x8371daff, "sg_copy_from_buffer" },
	{ 0xfd6d069a, "contig_page_data" },
	{ 0xe53d4df7, "__alloc_pages_nodemask" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x66b2a859, "nr_free_buffer_pages" },
	{ 0xd53cd7b5, "membank0_size" },
	{ 0x46ce1a07, "membank1_start" },
	{ 0x1e2db3e7, "mem_map" },
	{ 0xd5152710, "sg_next" },
	{ 0xf88c3301, "sg_init_table" },
	{ 0xcae95b2b, "mmc_start_req" },
	{ 0x71b70191, "_dev_info" },
	{ 0xefdd2345, "sg_init_one" },
	{ 0x93ca528d, "mmc_set_blocklen" },
	{ 0x46608fa0, "getnstimeofday" },
	{ 0xd241797, "mmc_can_trim" },
	{ 0xf2e5f9f7, "mmc_wait_for_req" },
	{ 0x7692e64b, "mmc_erase" },
	{ 0x116d6c7d, "mmc_can_erase" },
	{ 0x3cb46c78, "debugfs_remove" },
	{ 0x4bf5b0a0, "dev_err" },
	{ 0x3e28a19f, "debugfs_create_file" },
	{ 0xde853408, "mmc_release_host" },
	{ 0x87f76e75, "__mmc_claim_host" },
	{ 0x93fca811, "__get_free_pages" },
	{ 0x11a13e31, "_kstrtol" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0x37a0cba, "kfree" },
	{ 0x4fad95cf, "__free_pages" },
	{ 0xe6da44a, "set_normalized_timespec" },
	{ 0xceee21f4, "kmalloc_caches" },
	{ 0x542370a0, "kmem_cache_alloc" },
	{ 0x59e5070d, "__do_div64" },
	{ 0x16c38230, "mmc_wait_for_cmd" },
	{ 0x5f754e5a, "memset" },
	{ 0xfa2a45e, "__memzero" },
	{ 0x416b7751, "mmc_set_data_timeout" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x27e1a049, "printk" },
	{ 0xb4536ce7, "mmc_can_reset" },
	{ 0x72f78b35, "mmc_hw_reset_check" },
	{ 0xf7802486, "__aeabi_uidivmod" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0xf6ddf3b8, "mutex_unlock" },
	{ 0xd67319, "seq_printf" },
	{ 0x78e98764, "mutex_lock" },
	{ 0xeca8bcda, "single_open" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

