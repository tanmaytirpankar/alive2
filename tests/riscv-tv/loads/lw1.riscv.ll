@glob = external dso_local local_unnamed_addr global i32, align 4

define i32 @f() {
  %lv = load i32, ptr @glob, align 4
  ret i32 %lv
}