define dso_local noundef  i8 @f17(i8 noundef  %a, i8 noundef  %b) local_unnamed_addr #0 {
entry:
  %cmp = icmp ult i8 %a, %b
  %conv2 = zext i1 %cmp to i8
  %cmp5 = icmp ult i8 %b, %a
  %conv6 = zext i1 %cmp5 to i8
  %add = add i8 %b, %a
  %add9 = add i8 %add, %conv6
  %add10 = add i8 %add9, %conv2
  ret i8 %add10
}
