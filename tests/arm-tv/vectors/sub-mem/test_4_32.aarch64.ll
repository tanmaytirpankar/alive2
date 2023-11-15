@a = dso_local global <4 x i32> undef, align 1
@b = dso_local global <4 x i32> undef, align 1
@c = dso_local global <4 x i32> undef, align 1

define void @vector_sub_4_32() {
    %a = load <4 x i32>, ptr @a, align 1
    %b = load <4 x i32>, ptr @b, align 1
    %d = sub <4 x i32> %a, %b
    store <4 x i32> %d, ptr @c, align 1
    ret void
}
