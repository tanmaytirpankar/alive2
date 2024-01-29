@a = global ptr @b

@b = global ptr @a

define ptr @f() {
entry:
  ret ptr @a
}
