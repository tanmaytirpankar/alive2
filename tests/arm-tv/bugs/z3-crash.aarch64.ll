define i32 @non_speculatable_load_of_select_inner_inverted(i1 %0, i1 %1, ptr %2, ptr %3) {
  %5 = alloca i32, align 4
  store i32 0, ptr %5, align 4
  %6 = select i1 %0, ptr %3, ptr %5, !prof !0
  %7 = select i1 %1, ptr %6, ptr %2, !prof !0
  %8 = load i32, ptr %7, align 4
  ret i32 %8
}

!0 = !{!"branch_weights", i32 1, i32 99}
