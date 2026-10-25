"""Own child lifetimes independently of process names and root exit ordering.

@file owned_process_tree.py
@see docs/zz-reach2/architecture/archi-historical-debug.md
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md
The Windows job contains only a newly launched root and its descendants. Closing
its last handle retires descendants even when the root has already failed.
"""
import ctypes
from ctypes import wintypes
import os
import subprocess


class BasicLimits(ctypes.Structure):
    """Windows JOBOBJECT_BASIC_LIMIT_INFORMATION, including native pointer-size alignment."""

    _fields_ = [("process_time", ctypes.c_longlong), ("job_time", ctypes.c_longlong),
                ("flags", wintypes.DWORD), ("minimum_working_set", ctypes.c_size_t),
                ("maximum_working_set", ctypes.c_size_t), ("active_processes", wintypes.DWORD),
                ("affinity", ctypes.c_size_t), ("priority", wintypes.DWORD), ("scheduling", wintypes.DWORD)]


class IoCounters(ctypes.Structure):
    """Windows IO_COUNTERS embedded in the extended job limit structure."""

    _fields_ = [(name, ctypes.c_ulonglong) for name in
                ("read_operations", "write_operations", "other_operations", "read_bytes", "write_bytes", "other_bytes")]


class ExtendedLimits(ctypes.Structure):
    """Windows JOBOBJECT_EXTENDED_LIMIT_INFORMATION; memory limits remain unrestricted."""

    _fields_ = [("basic", BasicLimits), ("io", IoCounters), ("process_memory", ctypes.c_size_t),
                ("job_memory", ctypes.c_size_t), ("peak_process_memory", ctypes.c_size_t),
                ("peak_job_memory", ctypes.c_size_t)]


class OwnedWindowsJob:
    """Noninheritable kill-on-close job for exactly one launched process tree."""

    def __init__(self):
        """Reserve an unnamed job before launching; platform failures refuse ownership."""
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        self.kernel.CreateJobObjectW.argtypes = (ctypes.c_void_p, wintypes.LPCWSTR)
        self.kernel.CreateJobObjectW.restype = wintypes.HANDLE
        self.kernel.SetInformationJobObject.argtypes = (wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD)
        self.kernel.AssignProcessToJobObject.argtypes = (wintypes.HANDLE, wintypes.HANDLE)
        self.kernel.CloseHandle.argtypes = (wintypes.HANDLE,)
        self.handle = self.kernel.CreateJobObjectW(None, None)
        if not self.handle:
            raise ctypes.WinError(ctypes.get_last_error())
        limits = ExtendedLimits()
        limits.basic.flags = 0x2000  # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE; no inherited/breakaway ownership.
        if not self.kernel.SetInformationJobObject(self.handle, 9, ctypes.byref(limits), ctypes.sizeof(limits)):
            error = ctypes.WinError(ctypes.get_last_error())
            self.close()
            raise error

    def attach(self, child):
        """Assign the actual newly launched process handle, never a PID lookup.

        @param child Popen returned by this launch while the root is still owned.
        @return None; failure prevents the process from being treated as guarded.
        """
        if not self.kernel.AssignProcessToJobObject(self.handle, wintypes.HANDLE(int(child._handle))):
            raise ctypes.WinError(ctypes.get_last_error())

    def close(self):
        """Retire this job's remaining descendants and release its only owning handle."""
        if self.handle:
            self.kernel.CloseHandle(self.handle)
            self.handle = None


def close_owned_job(child):
    """Release only a job attached by our own launch, including after root completion.

    @param child Exact owned Popen object; ordinary/unrelated Popen objects are unchanged.
    """
    job = getattr(child, "artifact_owned_job", None)
    if job is not None:
        job.close()


def launch_owned_process(arguments, **options):
    """Launch one root with a noninheritable Windows job or an explicit POSIX session.

    @param arguments Structured command selected by the current caller.
    @param options Original Popen stream/environment/cwd options, without a shell wrapper.
    @return Popen carrying the exact ownership handle for later stop/reaping.
    """
    job = OwnedWindowsJob() if os.name == "nt" else None
    child = None
    try:
        if os.name != "nt":
            options.setdefault("start_new_session", True)
        child = subprocess.Popen(arguments, **options)
        child.artifact_owned_session = os.name != "nt" and options.get("start_new_session", False)
        if job is not None:
            job.attach(child)
            child.artifact_owned_job = job
        return child
    except BaseException:
        # Failed admission never leaves an unguarded root running. An already
        # attached job retires its descendants; the exact root is always reaped.
        if job is not None:
            job.close()
        if child is not None:
            if child.poll() is None:
                child.kill()
            child.wait(timeout=15)
        raise
