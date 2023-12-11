define i8 @getb1(i64 noundef %0, i32 noundef %1) {
  %3 = alloca i64, align 8
  store i64 %0, ptr %3, align 8
  %4 = sext i32 %1 to i64
  %5 = getelementptr inbounds i8, ptr %3, i64 %4
  %6 = load i8, ptr %5, align 1
  ret i8 %6
}
