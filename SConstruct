import os.path

# Build tools
asmBuilder = Builder(action = 'asm.py -o $TARGET $SOURCE', suffix=".o", src_suffix=[".s", ".asm"], single_source=True, src_builder="C")
cBuilder = Builder(action = 'ccpu-cc -I ../ccpu-libc/include -I ../ccpu-libsys/include -o $TARGET $SOURCE', suffix=".s", src_suffix=".c", single_source=True, source_scanner=CScanner)
def linkGenerator(source, target, env, for_signature):
    return f"link.py --layout=default-stack -m {target[1]} -o {target[0]} {' '.join(str(s) for s in source)}"

def loaderLinkGenerator(source, target, env, for_signature):
    return f"link.py --layout=loader.yaml --api-out={target[2]} -m {target[1]} -o {target[0]} {' '.join(str(s) for s in source)}"

def extensionLinkGenerator(source, target, env, for_signature):
    return f"link.py --slim --layout=extension.yaml --api-in={source[0]} -m {target[1]} -o {target[0]} {' '.join(str(s) for s in source[1:])}"

def appLinkGenerator(source, target, env, for_signature):
    return f"link.py --slim --layout=app.yaml -m {target[1]} -o {target[0]} {' '.join(str(s) for s in source)}"

def linkEmitter(target, source, env):
    t = target[0]
    base,ext = os.path.splitext(str(t))
    target.append(f"{base}.map")
    return target, source

def loaderLinkEmitter(target, source, env):
    t = target[0]
    base,ext = os.path.splitext(str(t))
    target.append(f"{base}.map")
    target.append(f"{base}.api")
    return target, source

linkBuilder = Builder(generator=linkGenerator, suffix=".bin", src_suffix=[".o", ".a"], src_builder="Asm", emitter=linkEmitter)
loaderLinkBuilder = Builder(generator=loaderLinkGenerator, suffix=".bin", src_suffix=[".o", ".a"], src_builder="Asm", emitter=loaderLinkEmitter)
extensionLinkBuilder = Builder(generator=extensionLinkGenerator, suffix=".ext", src_suffix=[".o", ".a"], src_builder="Asm", emitter=linkEmitter)
appLinkBuilder = Builder(generator=appLinkGenerator, suffix=".app", src_suffix=[".o", ".a"], src_builder="Asm", emitter=linkEmitter)

imageBuilder = Builder(action = './mkfat.sh data $SOURCES', suffix=".img", src_suffix=[".app", ".ext"])

env = Environment(BUILDERS = {
    'Asm' : asmBuilder,
    'C': cBuilder,
    'Bin': linkBuilder,
    'Loader': loaderLinkBuilder,
    'Extension': extensionLinkBuilder,
    'App': appLinkBuilder,
    'Image': imageBuilder}, ENV={'PATH': os.environ['PATH']})

env.PrependENVPath("PATH", "../ccpu/tools")
env["CC_DIR"] = "../ccpu-cc"
env["CPPPATH"] = ["lib", "../ccpu-libc/include", "../ccpu-libsys/include"]

# Libraries
bcdf = Split('''
    lib/bcdf.asm lib/bcdf_addsub.asm lib/bcdf_mul.asm lib/bcdf_div.asm lib/bcdf_print.asm
    ''')
c_runtime = ['$CC_DIR/ccpu-runtime/runtime.asm', '$CC_DIR/ccpu-runtime/divide32.asm', '$CC_DIR/ccpu-runtime/memcpy.asm']
rom_startup = ['$CC_DIR/ccpu-runtime/rom_startup.asm']
# os_startup = ['$CC_DIR/ccpu-runtime/os_startup.asm']
app_startup = ['$CC_DIR/ccpu-runtime/app_startup.asm']

libc = ['../ccpu-libc/libc.a']
libsys = ['../ccpu-libsys/libsys.a']
quasipixel = Split('lib/quasipixel.c lib/qp_render.asm')

# Apps

env.Bin('hello_rom', Split('hello.c') + c_runtime + rom_startup + libc + libsys)
env.App('hello', Split('hello.c') + c_runtime + app_startup + libc + libsys)

snake = Split('snake.c snake_maps.c')
env.Bin('snake_rom', snake + c_runtime + rom_startup + libc + libsys + quasipixel)
env.App('snake', snake + c_runtime + app_startup + libc + libsys + quasipixel)


env.App('ethtest', Split('ethtest.c') + c_runtime + app_startup + libc + libsys)

# env.Image('image', Split('''
#     main.app
#     maze.app
#     life.app
#     pong.app
#     edit.app
#     tetris.app
#     mndlbrt.app
#     plot.app
#     matrix.app
#     '''))
