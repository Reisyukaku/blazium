import os
import platform
from typing import TYPE_CHECKING

from methods import print_error, print_warning
from platform_methods import detect_arch, validate_arch

if TYPE_CHECKING:
    from SCons.Script.SConscript import SConsEnvironment


def get_name():
    return "Nintendo Switch"

def can_build():
    if "DEVKITPRO" not in os.environ:
        print("DEVKITPRO not defined in environment. NX diabled.")
        return False
    
    if not os.path.exists("{}/devkitA64".format(os.environ.get("DEVKITPRO"))):
        print("DEVKITA64 not found. NX disabled.")
        return False

    return True

def get_opts():
    from SCons.Variables import BoolVariable, EnumVariable
    return [
        BoolVariable("use_sanitizer", "Use LLVM compiler address sanitizer", False),
        BoolVariable("use_leak_sanitizer", "Use LLVM compiler memory leaks sanitizer (implies use_sanitizer)", False),
        BoolVariable("separate_debug_symbols", "Create a separate file containing debugging symbols", False),
        BoolVariable("touch", "Enable touch events", True),
        BoolVariable("vulkan", "Enable the vulkan rendering driver", False),
        BoolVariable("use_volk", "Use the volk library to load the Vulkan loader dynamically", False),
    ]


def get_doc_classes():
    return [
        "EditorExportPlatformNX",
    ]


def get_doc_path():
    return "doc_classes"


def get_flags():
    return {
        "arch": "arm64",
        "tools": False,

        #In switch-portlibs
        "builtin_zlib": False,
        "builtin_libpng": False,
        "builtin_freetype": False,
        "builtin_libogg": False,
        "builtin_libvorbis": False,
        "builtin_libtheora": False,
        "builtin_libwebp": False,
        "builtin_mbedtls": False,
        "builtin_wslay": False,
        "builtin_miniupnpc": False,
        "builtin_enet": False,
        "builtin_zstd": False,
        "builtin_pcre2": False,
        "builtin_bullet": False,
        "builtin_libvpx": False,
        "builtin_opus": False,

        #NOT in switch portlibs
        "builtin_embree": True,
        "builtin_squish": True,
        "builtin_glslang": True,
        "builtin_spirv_cross": True,
        "builtin_harfbuzz": True, #Is in portlibs but need lib icu
    }


def configure(env: "SConsEnvironment"):
    devkitpro = os.environ.get("DEVKITPRO")
    devkita64 = os.path.join(devkitpro, "devkitA64")

    env["ENV"]["DEVKITPRO"] = devkitpro
    updated_path = "{}/portlibs/switch/bin:{}/devkitA64/bin:".format(devkitpro, devkitpro) + os.environ["PATH"]
    env["ENV"]["PATH"] = updated_path
    os.environ["PATH"] = updated_path  # os environment has to be updated for subprocess calls

    toolchain_bin = os.path.join(devkita64, "bin")
    env["CC"] = os.path.join(toolchain_bin, "aarch64-none-elf-gcc")
    env["CXX"] = os.path.join(toolchain_bin, "aarch64-none-elf-g++")
    env["AR"] = os.path.join(toolchain_bin, "aarch64-none-elf-ar")
    env["RANLIB"] = os.path.join(toolchain_bin, "aarch64-none-elf-ranlib")
    env["AS"] = os.path.join(toolchain_bin, "aarch64-none-elf-as")

    env["vulkan"] = False
    env["use_volk"] = False

    arch = ["-march=armv8-a", "-mtune=cortex-a57", "-mtp=soft", "-fPIE"]

    #includes
    env.Prepend(CPPPATH=[os.path.join(devkitpro, "libnx", "include")])
    env.Prepend(CPPPATH=[os.path.join(devkitpro, "portlibs", "switch", "include")])
    env.Prepend(CPPPATH=["#platform/nx"])

    #libs
    env.Prepend(LIBPATH=[os.path.join(devkitpro, "libnx", "lib")])
    env.Prepend(LIBPATH=[os.path.join(devkitpro, "portlibs", "switch", "lib")])
    
    #flags
    env.Append(CCFLAGS = arch+[
        "-ffunction-sections",
        "-fdata-sections",
        "-Wall",
        "-O2",
        "-flax-vector-conversions"
    ])

    env.Prepend(CPPFLAGS=["-isystem", "{}/libnx/include".format(devkitpro)])
    env.Prepend(LINKFLAGS = arch+["-specs={}/libnx/switch.specs".format(devkitpro)])
    env.Append(CPPDEFINES=[
        "SQLITE_OMIT_LOAD_EXTENSION",
        "NX_ENABLED",
        "EGL_ENABLED",
        "GLES_ENABLED",
        "OPENGL_ENABLED",
        "LIBC_FILEIO_ENABLED",
        "PTHREAD_ENABLED",
        "PTHREAD_NO_RENAME",
        "__SWITCH__"
    ])
    env.Append(LIBS=["EGL", "GLESv2", "glapi", "drm_nouveau"])
    
    # freetype depends on libpng and zlib, so bundling one of them while keeping others
    # as shared libraries leads to weird issues
    if env["builtin_freetype"] or env["builtin_libpng"] or env["builtin_zlib"]:
        env["builtin_freetype"] = True
        env["builtin_libpng"] = True
        env["builtin_zlib"] = True

    if not env["builtin_freetype"]:
        env.ParseConfig("aarch64-none-elf-pkg-config freetype2 --cflags --libs")

    if not env["builtin_libpng"]:
        env.ParseConfig("aarch64-none-elf-pkg-config libpng --cflags --libs")

    if not env["builtin_bullet"]:
        # We need at least version 2.88
        import subprocess

        bullet_version = subprocess.check_output(["aarch64-none-elf-pkg-config", "bullet", "--modversion"]).strip()
        if str(bullet_version) < "2.88":
            # Abort as system bullet was requested but too old
            print(
                "Bullet: System version {0} does not match minimal requirements ({1}). Aborting.".format(
                    bullet_version, "2.88"
                )
            )
            sys.exit(255)
        env.ParseConfig("aarch64-none-elf-pkg-config bullet --cflags --libs")

    if not env["builtin_enet"]:
        env.ParseConfig("aarch64-none-elf-pkg-config libenet --cflags --libs")

    if not env["builtin_squish"] and env["tools"]:
        env.ParseConfig("aarch64-none-elf-pkg-config libsquish --cflags --libs")

    if not env["builtin_zstd"]:
        env.ParseConfig("aarch64-none-elf-pkg-config libzstd --cflags --libs")

    # Sound and video libraries
    # Keep the order as it triggers chained dependencies (ogg needed by others, etc.)

    if not env["builtin_libtheora"]:
        env["builtin_libogg"] = False  # Needed to link against system libtheora
        env["builtin_libvorbis"] = False  # Needed to link against system libtheora
        env.ParseConfig("aarch64-none-elf-pkg-config theora theoradec --cflags --libs")
    else:
        list_of_x86 = ["x86_64", "x86", "i386", "i586"]
        if any(platform.machine() in s for s in list_of_x86):
            env["x86_libtheora_opt_gcc"] = True

    if not env["builtin_libvpx"]:
        env.ParseConfig("aarch64-none-elf-pkg-config vpx --cflags --libs")

    if not env["builtin_libvorbis"]:
        env["builtin_libogg"] = False  # Needed to link against system libvorbis
        env.ParseConfig("aarch64-none-elf-pkg-config vorbis vorbisfile --cflags --libs")

    if not env["builtin_opus"]:
        env["builtin_libogg"] = False  # Needed to link against system opus
        env.ParseConfig("aarch64-none-elf-pkg-config opus opusfile --cflags --libs")

    if not env["builtin_libogg"]:
        env.ParseConfig("aarch64-none-elf-pkg-config ogg --cflags --libs")

    if not env["builtin_libwebp"]:
        env.ParseConfig("aarch64-none-elf-pkg-config libwebp --cflags --libs")

    if not env["builtin_mbedtls"]:
        # mbedTLS does not provide a pkgconfig config yet. See https://github.com/ARMmbed/mbedtls/issues/228
        env.Append(LIBS=["mbedtls", "mbedx509", "mbedcrypto"])

    if not env["builtin_wslay"]:
        env.ParseConfig("aarch64-none-elf-pkg-config libwslay --cflags --libs")

    if not env["builtin_miniupnpc"]:
        env.ParseConfig("aarch64-none-elf-pkg-config miniupnpc --cflags --libs")

    # On Linux wchar_t should be 32-bits
    # 16-bit library shouldn't be required due to compiler optimisations
    if not env["builtin_pcre2"]:
        env.ParseConfig("aarch64-none-elf-pkg-config libpcre2-32 --cflags --libs")
    
    env.Append(LIBS=["nx"])
