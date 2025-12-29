project('luo-agent', 'c',
  version : '0.1.0',
  license : 'GPL-2.0',
  default_options : [
    'warning_level=3',
    'c_std=gnu11',
    'optimization=s',
    'b_staticpic=true',
  ]
)

cc = meson.get_compiler('c')

# Dependencies: Request static versions
json_dep = dependency('json-c', static : true)
thread_dep = dependency('threads', static : true)

# Header Configuration
# Base include for internal common headers
inc_dirs = [include_directories('src/common')]

# Smart Fallback for UAPI
# If the system does not have the LUO kernel headers installed,
# fall back to our local copy in include/uapi/linux/liveupdate.h
if not cc.has_header('linux/liveupdate.h')
  message('System <linux/liveupdate.h> not found. Using local UAPI fallback.')
  inc_dirs += include_directories('include/uapi')
else
  message('Using system <linux/liveupdate.h>')
endif

# Common Sources
common_src = files('src/common/utils.c')

# Force static linking for the final binaries
# Note: You must have static glibc (libc.a) and static libjson-c.a installed
static_link_args = ['-static']

# Daemon (luod)
executable('luod',
  sources : ['src/daemon/main.c', common_src],
  dependencies : [json_dep, thread_dep],
  include_directories : inc_dirs,
  link_args : static_link_args,
  install : true,
  install_dir : '/usr/sbin'
)

# Admin Tool (luoctl)
executable('luoctl',
  sources : ['src/ctl/main.c', common_src],
  dependencies : [json_dep],
  include_directories : inc_dirs,
  link_args : static_link_args,
  install : true,
  install_dir : '/usr/bin'
)

# Systemd Unit
install_data('systemd/luod.service',
  install_dir : '/lib/systemd/system'
)
