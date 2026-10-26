/*
 * Cint.c - hand disassembly of Cint.lib (member .\Release\Cint.obj, x86 COFF, VC6 /O2 Release).
 * The member was extracted with a small Python reader for the "!<arch>" format; each code section
 * (COMDAT .text) is listed with its relocations resolved to symbol names, then as pseudo-C.
 * Layout from the code: this+0 int *m_pValue, this+4 int m_iKeyValue, this+8 unsigned long old.
 */

/* ??0CInt@@QAE@XZ  CInt::CInt()
 *   push esi; push 4 (PAGE_READWRITE); push 1000h (MEM_COMMIT); push 4; mov esi,ecx; push 0
 *   call [__imp__VirtualAlloc@16]; mov [esi],eax; mov dword [eax],0
 *   call _rand; mov ecx,[esi]; mov [esi+4],eax; mov edx,[ecx]; xor edx,eax; mov eax,esi
 *   mov [ecx],edx; pop esi; ret
 */
CInt *CInt_ctor(CInt *this)
{
    this->m_pValue = VirtualAlloc(NULL, 4, MEM_COMMIT, PAGE_READWRITE);
    *this->m_pValue = 0;
    this->m_iKeyValue = rand();
    *this->m_pValue ^= this->m_iKeyValue;
    return this;
}

/* ??1CInt@@QAE@XZ  CInt::~CInt()
 *   mov eax,[ecx]; push 4000h (MEM_DECOMMIT); push 4; push eax; call [__imp__VirtualFree@12]; ret
 */
void CInt_dtor(CInt *this) { VirtualFree(this->m_pValue, 4, MEM_DECOMMIT); }

/* ?Get@CInt@@QAEHXZ  int CInt::Get()
 *   mov edx,[ecx]; mov eax,[ecx+4]; xor eax,[edx]; ret
 */
int CInt_Get(CInt *this) { return *this->m_pValue ^ this->m_iKeyValue; }

/* ?Set@CInt@@QAEXABH@Z  void CInt::Set(const int &)
 *   push ebx; push esi; mov esi,ecx; mov ebx,[__imp__VirtualProtect@16]; push edi
 *   mov eax,[esi]; lea edi,[esi+8]; push edi; push 4; push 4; push eax; call ebx
 *   test eax,eax; je done
 *   call _rand; mov ecx,[esp+10h]; mov [esi+4],eax; push edi; push 2 (PAGE_READONLY)
 *   mov edx,[ecx]; push 4; xor edx,eax; mov eax,[esi]; mov [eax],edx; mov ecx,[esi]; push ecx
 *   call ebx
 * done: pop edi; pop esi; pop ebx; ret 4
 */
void CInt_Set(CInt *this, const int *piValue)
{
    if (VirtualProtect(this->m_pValue, 4, PAGE_READWRITE, &this->old)) {
        this->m_iKeyValue = rand();
        *this->m_pValue = *piValue ^ this->m_iKeyValue;
        VirtualProtect(this->m_pValue, 4, PAGE_READONLY, &this->old);
    }
}

/* ??4CInt@@QAEAAV0@ABH@Z  CInt &CInt::operator=(const int &)
 *   mov eax,[esp+4]; push esi; mov esi,ecx; push eax; call ?Set@CInt@@QAEXABH@Z; mov eax,esi; pop esi; ret 4
 */
CInt *CInt_assign(CInt *this, const int *piValue) { CInt_Set(this, piValue); return this; }

/* ??BCInt@@QAEHXZ  CInt::operator int()
 *   jmp ?Get@CInt@@QAEHXZ
 */
int CInt_to_int(CInt *this) { return CInt_Get(this); }

/* ??8CInt@@QAE_NABH@Z  bool CInt::operator==(const int &)   (declaration commented out in Cint.h)
 *   push esi; call ?Get@CInt@@QAEHXZ; mov edx,[esp+8]; xor ecx,ecx; mov esi,[edx]; cmp eax,esi
 *   pop esi; sete cl; mov al,cl; ret 4
 */
bool CInt_eq(CInt *this, const int *p) { return CInt_Get(this) == *p; }

/* ??9CInt@@QAE_NABH@Z  bool CInt::operator!=(const int &)   (declaration commented out in Cint.h)
 *   same as above with setne
 */
bool CInt_ne(CInt *this, const int *p) { return CInt_Get(this) != *p; }

/* The remaining sections (.CRT$XCU entries $S18/$S24/$S31 and $E14..$E30) are the static
 * initialisers that <iostream>, included by Cint.h, puts into every object file:
 * std::ios_base::Init, std::_Winit and std::ctype<unsigned short>::id. They are not part of CInt.
 */
