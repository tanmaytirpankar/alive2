%s = type { ptr }

@p = global %s { ptr @f }

define ptr @f() {
  ret ptr @p
}
