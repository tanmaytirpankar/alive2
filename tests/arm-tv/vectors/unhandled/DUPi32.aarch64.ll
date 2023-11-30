define i1 @foo(i64 %0) {
  %2 = bitcast i64 %0 to <2 x float>
  %3 = extractelement <2 x float> %2, i32 0
  %4 = extractelement <2 x float> %2, i32 1
  %5 = fcmp uno float %3, %4
  ret i1 %5
}
