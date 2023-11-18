define i32 @non_speculatable_load_of_select_inner_inverted(i1 %0, i1 %1, ptr %2, ptr %3) {
  %5 = alloca i32, align 4
  store i32 0, ptr %5, align 4
  %6 = select i1 %0, ptr %3, ptr %5, !prof !0
  %7 = select i1 %1, ptr %6, ptr %2, !prof !0
  %8 = load i32, ptr %7, align 4
  ret i32 %8
}

!0 = !{!"branch_weights", i32 1, i32 99}
regehr@ohm:~/tmp$ cat bar.ll

define i32 @non_speculatable_load_of_select_inner_inverted(i1 %0, i1 %1, ptr %2, ptr %3) local_unnamed_addr {
arm_tv_entry:
  %stack = tail call dereferenceable(640) ptr @myalloc(i32 640)
  %4 = getelementptr inbounds i8, ptr %stack, i64 512
  %5 = ptrtoint ptr %4 to i64
  %6 = ptrtoint ptr %2 to i64
  %7 = ptrtoint ptr %3 to i64
  %a2_1 = add i64 %5, -16
  %a3_1 = add i64 %5, -4
  %8 = inttoptr i64 %a2_1 to ptr
  %9 = getelementptr i8, ptr %8, i64 12
  store i32 0, ptr %9, align 1
  %a6_7 = select i1 %0, i64 %7, i64 %a3_1
  %a8_7 = select i1 %1, i64 %a6_7, i64 %6
  %10 = inttoptr i64 %a8_7 to ptr
  %a9_1 = load i32, ptr %10, align 1
  ret i32 %a9_1
}

; Function Attrs: allockind("alloc") allocsize(0)
declare nonnull ptr @myalloc(i32) local_unnamed_addr #0

attributes #0 = { allockind("alloc") allocsize(0) }
