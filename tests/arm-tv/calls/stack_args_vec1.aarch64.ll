declare <8 x i8> @g(<8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>, <8 x i8>)

define <8 x i8> @f() {
  %x = call <8 x i8> @g(<8 x i8> <i8 2, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>,
       	       	     	<8 x i8> <i8 1, i8 2, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 2, i8 1, i8 1, i8 1, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 1, i8 2, i8 1, i8 1, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 1, i8 1, i8 2, i8 1, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 1, i8 1, i8 1, i8 2, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 2, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 2>,
			<8 x i8> <i8 3, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 3, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 3, i8 1, i8 1, i8 1, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 1, i8 3, i8 1, i8 1, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 1, i8 1, i8 3, i8 1, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 1, i8 1, i8 1, i8 3, i8 1, i8 1>,
			<8 x i8> <i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 3, i8 1>)
  ret <8 x i8> %x
}