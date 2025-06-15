target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"
target triple = "riscv64"

%struct.tf_0_struct_2 = type { i32, %struct.tf_0_struct_1, i64, i16, i32, %struct.tf_0_struct_1 }
%struct.tf_0_struct_1 = type { i16, i16, i32, i32, i32 }

@tf_0_array_8 = external global [10 x %struct.tf_0_struct_2]
@tf_0_ptr_12 = external global ptr

define void @tf_0_foo(i16 %0, i1 %cmp23) {
entry:
  store ptr @tf_0_array_8, ptr @tf_0_ptr_12, align 8
  ret void
}
