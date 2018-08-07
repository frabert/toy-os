[GLOBAL task_switch]
task_switch:
  push ebx ;
  push esi ; Save the leaving task's registers
  push edi ;
  push ebp ;

  mov edi, [esp + (4+1)*4] ; Address of current task's descriptor
  mov esi, [esp + (4+2)*4] ; Address of next task's descriptor

  mov [edi + 5*4], esp ; Save old task's stack
  mov esp, [esi + 5*4]
  mov eax, [esi + 2*4]
  mov ecx, cr3
  cmp eax, ecx
  je .endSwitch
  mov cr3, eax

.endSwitch:
  pop ebp
  pop edi
  pop esi
  pop ebx

  ret