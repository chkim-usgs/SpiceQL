import ctypes
import numpy as np
import os
import sys

def _find_lib():
    # Load library
    try:
        from . import _pyspiceql as mod
        
        lib = ctypes.CDLL(mod.__file__)
        if hasattr(lib, "writeCkFromBuffers"):
            return lib, mod.__file__, []
    except (ImportError, AttributeError, OSError) as e:
        # Log the error to your console/terminal for debugging
        print(f"DEBUG: Internal import failed: {e}")
        pass

    # Manual search and load library
    try:
        current_dir = os.path.dirname(os.path.abspath(__file__))
        # Look for the .so or .dylib file in the same folder as ck_writer.py
        for f in os.listdir(current_dir):
            if f.startswith("_pyspiceql") and (f.endswith(".so") or f.endswith(".dylib")):
                path = os.path.join(current_dir, f)
                lib = ctypes.CDLL(path)
                if hasattr(lib, "writeCkFromBuffers"):
                    return lib, path, []
    except Exception as e:
        print(f"DEBUG: Manual directory search failed: {e}")

    return None, None, []


_ck_lib, _ck_lib_path, _ck_lib_search_dirs = _find_lib()

if _ck_lib is not None:
    try:
        _ck_lib.writeCkFromBuffers.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_double),
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_double),
            ctypes.c_size_t,
            ctypes.c_int,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_double),
            ctypes.c_size_t,
            ctypes.c_char_p,
        ]
        _ck_lib.writeCkFromBuffers.restype = ctypes.c_int
        if hasattr(_ck_lib, "writeCkFromBuffersLastError"):
            _ck_lib.writeCkFromBuffersLastError.restype = ctypes.c_char_p
            _ck_lib.writeCkFromBuffersLastError.argtypes = []
        _ck_lib_ok = True
    except AttributeError:
        _ck_lib_ok = False
else:
    _ck_lib_ok = False


def write_ck(
    path,
    quats,
    times,
    body_code,
    reference_frame,
    segment_id,
    sclk,
    lsk,
    angular_velocities=None,
    comment="",
):
    if not _ck_lib_ok:
        dirs = _ck_lib_search_dirs
        hint = ""
        if dirs:
            hint = " Searched: " + ", ".join(dirs) + ". "
        raise RuntimeError(
            "writeCkFromBuffers not found. Rebuild SpiceQL and ensure libSpiceQL is on the library path."
            + hint
        )

    quats = np.ascontiguousarray(np.asarray(quats, dtype=np.float64))
    times = np.ascontiguousarray(np.asarray(times, dtype=np.float64))
    n = quats.shape[0]
    if quats.ndim != 2 or quats.shape[1] != 4:
        raise ValueError("quats must have shape (n, 4)")
    if times.shape[0] != n:
        raise ValueError("times length must match quats rows")

    if angular_velocities is not None and len(angular_velocities) != 0:
        angular_velocities = np.ascontiguousarray(np.asarray(angular_velocities, dtype=np.float64))
        if angular_velocities.shape[0] != n or angular_velocities.shape[1] != 3:
            raise ValueError("angular_velocities must have shape (n, 3)")
        av_ptr = angular_velocities.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
        n_av = n
    else:
        av_ptr = None
        n_av = 0

    def _b(s):
        """Convert value to bytes"""
        if s is None:
            return b""
        if isinstance(s, str):
            return s.encode("utf-8")
        return str(s).encode("utf-8")

    # Accept str or list of str (multiple SCLK kernels) for sclk, comma deliminated
    sclk_arg = ",".join(sclk) if isinstance(sclk, (list, tuple)) else sclk

    rc = _ck_lib.writeCkFromBuffers(
        _b(path),
        quats.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
        n,
        times.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
        n,
        int(body_code),
        _b(reference_frame),
        _b(segment_id),
        _b(sclk_arg),
        _b(lsk),
        av_ptr,
        n_av,
        _b(comment),
    )
    if rc != 0:
        err = "unknown error"
        if hasattr(_ck_lib, "writeCkFromBuffersLastError"):
            msg = _ck_lib.writeCkFromBuffersLastError()
            if msg:
                err = msg.decode("utf-8") if isinstance(msg, bytes) else msg
        raise RuntimeError(f"writeCkFromBuffers failed: {err}")
