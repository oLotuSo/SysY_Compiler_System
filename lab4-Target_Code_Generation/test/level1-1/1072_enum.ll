define i32 @main() {
B38:
  %t42 = alloca i32, align 4
  %t41 = alloca i32, align 4
  %t40 = alloca i32, align 4
  %t39 = alloca i32, align 4
  store i32 0, i32* %t39, align 4
  store i32 0, i32* %t40, align 4
  store i32 0, i32* %t41, align 4
  br label %B45
B45:                               	; preds = %B38, %B50
  %t7 = load i32, i32* %t39, align 4
  %t8 = icmp slt i32 %t7, 21
  br i1 %t8, label %B43, label %B47
B43:                               	; preds = %B45
  br label %B51
B47:                               	; preds = %B45
  br label %B44
B51:                               	; preds = %B43, %B56
  %t9 = load i32, i32* %t40, align 4
  %t10 = load i32, i32* %t39, align 4
  %t11 = sub i32 101, %t10
  %t12 = icmp slt i32 %t9, %t11
  br i1 %t12, label %B49, label %B53
B44:                               	; preds = %B47
  ret i32 0
B49:                               	; preds = %B51
  %t14 = load i32, i32* %t39, align 4
  %t15 = sub i32 100, %t14
  %t16 = load i32, i32* %t40, align 4
  %t17 = sub i32 %t15, %t16
  store i32 %t17, i32* %t41, align 4
  %t18 = load i32, i32* %t39, align 4
  %t19 = mul i32 5, %t18
  %t20 = load i32, i32* %t40, align 4
  %t21 = mul i32 1, %t20
  %t22 = add i32 %t19, %t21
  %t23 = load i32, i32* %t41, align 4
  %t24 = sdiv i32 %t23, 2
  %t25 = add i32 %t22, %t24
  %t26 = icmp eq i32 %t25, 100
  br i1 %t26, label %B55, label %B58
B53:                               	; preds = %B51
  br label %B50
B55:                               	; preds = %B49
  %t27 = load i32, i32* %t39, align 4
  call void @putint(i32 %t27)
  %t28 = load i32, i32* %t40, align 4
  call void @putint(i32 %t28)
  %t29 = load i32, i32* %t41, align 4
  call void @putint(i32 %t29)
  store i32 10, i32* %t42, align 4
  %t31 = load i32, i32* %t42, align 4
  call void @putch(i32 %t31)
  br label %B56
B58:                               	; preds = %B49
  br label %B56
B50:                               	; preds = %B53
  %t36 = load i32, i32* %t39, align 4
  %t37 = add i32 %t36, 1
  store i32 %t37, i32* %t39, align 4
  br label %B45
B56:                               	; preds = %B55, %B58
  %t33 = load i32, i32* %t40, align 4
  %t34 = add i32 %t33, 1
  store i32 %t34, i32* %t40, align 4
  br label %B51
}
declare void @putint(i32)
declare void @putch(i32)
