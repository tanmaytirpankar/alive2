@a = dso_local global <4 x i64> undef, align 1
@b = dso_local global <4 x i64> undef, align 1
@c = dso_local global <4 x i64> undef, align 1

define void @vector_add_4_64() {
    %a = load <4 x i64>, ptr @a, align 1
    %b = load <4 x i64>, ptr @b, align 1
    %d = add <4 x i64> %a, %b
    store <4 x i64> %d, ptr @c, align 1
    ret void
}