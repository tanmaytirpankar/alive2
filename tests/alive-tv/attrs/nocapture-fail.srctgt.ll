; ERROR: Source is more defined than target

@x = global ptr null

define void @src(ptr captures(none) %p) {
  %poison = getelementptr inbounds i8, ptr null, i32 1
  store ptr %poison, ptr @x
  ret void
}

define void @tgt(ptr captures(none) %p) {
  store ptr %p, ptr @x
  ret void
}
