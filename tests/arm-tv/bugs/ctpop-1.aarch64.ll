declare i32 @llvm.ctpop.i32(i32)

define signext i32 @func32s(i32 signext %0) {
  %2 = tail call i32 @llvm.ctpop.i32(i32 %0)
  ret i32 %2
}


