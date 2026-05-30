# -----------------------------------------------------------------------------
# 
# Flash firmware image and attach GDB session
# -----------------------------------------------------------------------------
gdb-multiarch \
  -ex "set confirm off" \
  -ex "target remote 100.84.168.69:3333" \
  -ex "file $1" \
  -ex "load" \
  -ex "monitor reset init" \
  -ex "continue"
