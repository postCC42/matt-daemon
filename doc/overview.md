# Overview
## Signal handling

**Non-Handleable Signals:** The comments mention `SIGKILL` and `SIGSTOP`, which cannot be caught, blocked, or ignored by processes. This is by design, ensuring that these signals can always be used to control processes at the system level.  

**Debugging Signal:** `SIGTRAP` is typically used in debugging scenarios. The comment suggests avoiding catching this signal to not interfere with debuggers that rely on it.

**Termination Signals:** `SIGTERM`, `SIGHUP`, `SIGQUIT` and `SIGINT` are used to request a process to terminate. The comment suggests catching these signals to perform cleanup operations before exiting.