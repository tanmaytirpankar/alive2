define dso_local noundef  i32 @f5(i32 noundef  %a, i32 noundef  %b) local_unnamed_addr #0 {
entry:
  %cmp = icmp slt i32 %a, %b
  %conv = zext i1 %cmp to i32
  %cmp1 = icmp slt i32 %b, %a
  %conv2 = zext i1 %cmp1 to i32
  %add = add i32 %b, %a
  %add3 = add i32 %add, %conv2
  %add4 = add i32 %add3, %conv
  ret i32 %add4
}
