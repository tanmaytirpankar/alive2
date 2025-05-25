; CHECK: 1 incorrect transformations

define dso_local noundef zeroext i8 @f13(i8 noundef zeroext %a, i8 noundef zeroext %b) local_unnamed_addr #0 {
entry:
  %sub = sub i8 %a, %b
  ret i8 %sub
}
