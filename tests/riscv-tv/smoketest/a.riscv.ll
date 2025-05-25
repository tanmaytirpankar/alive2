define dso_local  i32 @f1(i32 noundef  %a, i32 noundef  %b) local_unnamed_addr #0 {
entry:
  %sub = sub nsw i32 %a, %b
  ret i32 %sub
}
