import os

imageBuilder = Builder(action = './mkfat.sh data gemplex/GEMPLEX $SOURCES', suffix=".img", src_suffix=[".app", ".ext"])

env = Environment(
    CC='ccpu-cc',
    LINK="link.py",
    AS="asm.py",
    CPPPATH=['#.', os.path.abspath("../ccpu-libc/include"), os.path.abspath("../ccpu-libsys/include")],
    LIBPATH=[os.path.abspath("../ccpu-libc"), os.path.abspath("../ccpu-libsys")],
    ENV={'PATH': os.environ['PATH'] },
    CC_DIR=os.path.abspath("../ccpu-cc"),
    ROOT=os.path.abspath(".")
)
env.PrependENVPath("PATH", os.path.abspath("../ccpu/tools"))
env.Append(BUILDERS={'Image': imageBuilder})

app_layout = os.path.abspath("app.yaml")

app_env = env.Clone(LINKFLAGS = ["--slim", "--layout=" + app_layout], PROGSUFFIX=".app")
rom_env = env.Clone(LINKFLAGS = ["--layout=default-stack"], PROGSUFFIX=".bin")

c_runtime = env.Object(['$CC_DIR/ccpu-runtime/runtime.asm', '$CC_DIR/ccpu-runtime/divide32.asm', '$CC_DIR/ccpu-runtime/memcpy.asm'])
app_startup = env.Object(['$CC_DIR/ccpu-runtime/app_startup.asm'])
rom_startup = env.Object(['$CC_DIR/ccpu-runtime/rom_startup.asm'])
ipcfg = env.Object(Split('lib/ipcfg.c'))

Export("env", "app_env", "rom_env", "c_runtime", "app_startup", "rom_startup", "ipcfg")

SConscript("fatos/SConscript")
apps = SConscript("SConscript")
httpd_app = SConscript("httpd/SConscript")
lanpong_app = SConscript("lanpong/SConscript")
dhcpc_app = SConscript("dhcpc/SConscript")
gemplex_app = SConscript("gemplex/SConscript")
mandelbrot_app = SConscript("mandelbrot/SConscript")

env.Image('image', apps + lanpong_app + dhcpc_app + httpd_app + gemplex_app + mandelbrot_app)
