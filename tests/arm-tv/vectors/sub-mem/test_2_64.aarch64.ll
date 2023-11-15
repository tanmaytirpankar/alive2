@a = dso_local global <2 x i64> undef, align 1
@b = dso_local global <2 x i64> undef, align 1
@c = dso_local global <2 x i64> undef, align 1

define void @vector_sub_2_64() {
    %a = load <2 x i64>, ptr @a, align 1
    %b = load <2 x i64>, ptr @b, align 1
    %d = sub <2 x i64> %a, %b
    store <2 x i64> %d, ptr @c, align 1
    ret void
}
