
@fp = dso_local local_unnamed_addr global ptr null, align 8

define dso_local void @go(i32 noundef %0)  {
  %2 = icmp eq i32 %0, 0
  %3 = select i1 %2, ptr @f2, ptr @f1
  store ptr %3, ptr @fp, align 8
  ret void
}

declare dso_local void @f1(i32 noundef) 

declare dso_local void @f2(i32 noundef) 

