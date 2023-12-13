Import("env", "app_env", "rom_env", "c_runtime", "app_startup", "rom_startup")

quasipixel = env.Object(Split('$ROOT/lib/quasipixel.c $ROOT/lib/qp_render.asm'))

hello = env.Object("hello.c")

app_env.Program("hello", hello + c_runtime + app_startup, LIBS=['c', 'sys'])
rom_env.Program("hello", hello + c_runtime + rom_startup, LIBS=['c', 'sys'])

snake = env.Object(Split('snake.c snake_maps.c'))

rom_env.Program('snake_rom', snake + c_runtime + rom_startup + quasipixel, LIBS=['c', 'sys'])
snake_app = app_env.Program('snake', snake + c_runtime + app_startup + quasipixel, LIBS=['c', 'sys'])

shell_app = app_env.Program('shell', Split('shell.c lib/more.c') + c_runtime + app_startup, LIBS=['c', 'sys'])

ethtest_app = app_env.Program('ethtest', Split('ethtest.c') + c_runtime + app_startup, LIBS=['c', 'sys'])

Return("snake_app shell_app ethtest_app")
