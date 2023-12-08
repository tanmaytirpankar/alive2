define i8 @bitcast_scalar_index_variable(i32 %0, i64 %1) {
  %3 = bitcast i32 %0 to <4 x i8>
  %4 = extractelement <4 x i8> %3, i64 %1
  ret i8 %4
}
