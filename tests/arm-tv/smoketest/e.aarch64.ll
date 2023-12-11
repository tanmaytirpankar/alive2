define i8 @getb2(i64 noundef %0, i32 noundef %1) {
  %3 = shl nsw i32 %1, 3
  %4 = zext nneg i32 %3 to i64
  %5 = lshr i64 %0, %4
  %6 = trunc i64 %5 to i8
  ret i8 %6
}
