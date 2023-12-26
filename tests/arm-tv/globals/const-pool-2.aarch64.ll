; ModuleID = '<stdin>'
source_filename = "<stdin>"

define <2 x i32> @test40vec2(i1 %0) {
  %2 = select i1 %0, <2 x i32> <i32 1000, i32 2500>, <2 x i32> <i32 10, i32 30>
  %3 = and <2 x i32> %2, <i32 123, i32 333>
  ret <2 x i32> %3
}
