[GLOBAL spinlock_acquire]
spinlock_acquire:
  mov edx, [esp + 4] ; EDX now containes the address of the spinlock variable

  .spin:
  pause
  mov ecx, 1
  xchg ecx, [edx]
  test ecx, ecx
  jnz .spin

  ret

[GLOBAL spinlock_release]
spinlock_release:
  mov edx, [esp + 4]
  xor ecx, ecx
  xchg ecx, [edx]

  ret