?RemoveNonThreating@PossibleAttackTargetsValue@v20@@QEAAXAEAV?$list@_KV?$allocator@_K@std@@@std@@_N@Z PROC ; v20::PossibleAttackTargetsValue::RemoveNonThreating, COMDAT

; 35   : {

$LN508:
	mov	QWORD PTR [rsp+16], rbx
	mov	BYTE PTR [rsp+24], r8b
	mov	QWORD PTR [rsp+8], rcx
	push	rbp
	push	rsi
	push	rdi
	push	r12
	push	r13
	push	r14
	push	r15
	sub	rsp, 80					; 00000050H
	movaps	XMMWORD PTR [rsp+64], xmm6
	mov	rsi, rdx
	mov	r14, rcx
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 353  :     _List_val() noexcept : _Myhead(), _Mysize(0) {} // initialize data

	xor	edi, edi
	mov	QWORD PTR breakableCC$[rsp+8], rdi
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
	mov	rbp, rax
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1852 :         _Construct_in_place(_Newhead->_Next, _Newhead);

	mov	QWORD PTR [rax], rax

; 1853 :         _Construct_in_place(_Newhead->_Prev, _Newhead);

	mov	QWORD PTR [rax+8], rax

; 1854 :         _Mypair._Myval2._Myhead = _Newhead;

	mov	QWORD PTR breakableCC$[rsp], rax

; 353  :     _List_val() noexcept : _Myhead(), _Mysize(0) {} // initialize data

	xor	r13d, r13d
	mov	QWORD PTR unBreakableCC$[rsp+8], r13
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
	mov	r15, rax
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1852 :         _Construct_in_place(_Newhead->_Next, _Newhead);

	mov	QWORD PTR [rax], rax

; 1853 :         _Construct_in_place(_Newhead->_Prev, _Newhead);

	mov	QWORD PTR [rax+8], rax

; 1854 :         _Mypair._Myval2._Myhead = _Newhead;

	mov	QWORD PTR unBreakableCC$[rsp], rax

; 1105 :         return iterator(_Mypair._Myval2._Myhead->_Next, _STD addressof(_Mypair._Myval2));

	mov	rcx, QWORD PTR [rsi]

; 37   :     _List_unchecked_const_iterator(_Nodeptr _Pnode, const _Mylist* _Plist) noexcept : _Ptr(_Pnode) {

	mov	rbx, QWORD PTR [rcx]

; 202  :         return !(*this == _Right);

	mov	rdx, 768614336404564650			; 0aaaaaaaaaaaaaaaH
	cmp	rbx, rcx
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 39   :     for(std::list<ObjectGuid>::iterator tIter = targets.begin(); tIter != targets.end();)

	je	$LN482@RemoveNonT
	movss	xmm6, DWORD PTR __real@42700000
	npad	5
$LL4@RemoveNonT:

; 40   :     {
; 41   :         Unit* target = ai->GetUnit(*tIter);

	mov	r12, rbx
	mov	r14, QWORD PTR [r14]
	mov	rdi, QWORD PTR [rbx+16]
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\source\tests\playerbots\v20_targets\main.cpp

; 79   :         world->event(1, guid);

	mov	r8, rdi
	mov	edx, 1
	mov	rcx, QWORD PTR [r14]
	call	?event@World@@QEAAXI_K@Z		; World::event

; 80   :         if (!guid) return nullptr;

	test	rdi, rdi
	je	SHORT $LN154@RemoveNonT

; 81   :         assert(guid <= world->units.size());

	cmp	rdi, 32					; 00000020H
	jbe	SHORT $LN157@RemoveNonT
	mov	r8d, 81					; 00000051H
	lea	rdx, OFFSET FLAT:??_C@_1BBE@NHKIELNI@?$AAC?$AA?3?$AA?2?$AAU?$AAs?$AAe?$AAr?$AAs?$AA?2?$AAu?$AAs?$AAe?$AAr?$AA?2?$AAD@
	lea	rcx, OFFSET FLAT:??_C@_1DI@ELKMIOFL@?$AAg?$AAu?$AAi?$AAd?$AA?5?$AA?$DM?$AA?$DN?$AA?5?$AAw?$AAo?$AAr?$AAl?$AAd?$AA?9?$AA?$DO@
	call	QWORD PTR __imp__wassert
$LN157@RemoveNonT:

; 82   :         Unit* unit = &world->units[guid - 1];

	lea	rdi, QWORD PTR [rdi+rdi*2]
	mov	rax, QWORD PTR [r14]
	lea	rdi, QWORD PTR [rdi-3]
	lea	rdi, QWORD PTR [rax+rdi*8]

; 83   :         unit->ignoreCalls = 0;

	mov	DWORD PTR [rdi+16], 0
$LN154@RemoveNonT:

; 96   :         assert(player == bot && range == 60.0f && ignoreCC && !checkAttackerValid);

	movss	xmm0, DWORD PTR ?sPlayerbotAIConfig@@3UConfig@@A
	ucomiss	xmm0, xmm6
	jp	SHORT $LN502@RemoveNonT
	je	SHORT $LN164@RemoveNonT
$LN502@RemoveNonT:
	mov	r8d, 96					; 00000060H
	lea	rdx, OFFSET FLAT:??_C@_1BBE@NHKIELNI@?$AAC?$AA?3?$AA?2?$AAU?$AAs?$AAe?$AAr?$AAs?$AA?2?$AAu?$AAs?$AAe?$AAr?$AA?2?$AAD@
	lea	rcx, OFFSET FLAT:??_C@_1IG@PLCGPAAI@?$AAp?$AAl?$AAa?$AAy?$AAe?$AAr?$AA?5?$AA?$DN?$AA?$DN?$AA?5?$AAb?$AAo?$AAt?$AA?5?$AA?$CG@
	call	QWORD PTR __imp__wassert
$LN164@RemoveNonT:

; 97   :         ai->world->event(2, target ? target->guid : 0);

	mov	r14, QWORD PTR this$[rsp]
	mov	rax, QWORD PTR [r14]
	mov	rcx, QWORD PTR [rax]
	mov	edx, 2
	test	rdi, rdi
	je	$LN165@RemoveNonT
	mov	r8, QWORD PTR [rdi]
	call	?event@World@@QEAAXI_K@Z		; World::event

; 98   :         return target && target->state.valid;

	cmp	BYTE PTR [rdi+8], 0
	je	$LN167@RemoveNonT

; 101  :         assert(player == bot); ai->world->event(3, target->guid);

	mov	rcx, QWORD PTR [r14]
	mov	r8, QWORD PTR [rdi]
	mov	edx, 3
	mov	rcx, QWORD PTR [rcx]
	call	?event@World@@QEAAXI_K@Z		; World::event

; 102  :         return target->ignoreCalls++ == 0 ? target->state.ignoreFirst : target->state.ignoreSecond;

	mov	ecx, DWORD PTR [rdi+16]
	lea	eax, DWORD PTR [rcx+1]
	mov	DWORD PTR [rdi+16], eax
	test	ecx, ecx
	lea	rax, QWORD PTR [rdi+9]
	je	SHORT $LN222@RemoveNonT
	lea	rax, QWORD PTR [rdi+10]
$LN222@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 48   :         else if (!HasIgnoreCCRti(target, bot) && HasBreakableCC(target, bot))

	cmp	BYTE PTR [rax], 0
	jne	$LN498@RemoveNonT
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\source\tests\playerbots\v20_targets\main.cpp

; 105  :         assert(player == bot); ai->world->event(4, target->guid); return target->state.breakable;

	mov	rcx, QWORD PTR [r14]
	mov	r8, QWORD PTR [rdi]
	mov	edx, 4
	mov	rcx, QWORD PTR [rcx]
	call	?event@World@@QEAAXI_K@Z		; World::event
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 48   :         else if (!HasIgnoreCCRti(target, bot) && HasBreakableCC(target, bot))

	cmp	BYTE PTR [rdi+11], 0
	je	SHORT $LN498@RemoveNonT

; 49   :         {
; 50   :             std::list<ObjectGuid>::iterator tIter2 = tIter;

	mov	rax, rbx
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 164  :         this->_Ptr = this->_Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]

; 1560 :         if (this != _STD addressof(_Right) || (_UWhere != _UFirst && _UWhere != _ULast)) {

	lea	rcx, QWORD PTR breakableCC$[rsp]
	cmp	rcx, rsi
	jne	SHORT $LN496@RemoveNonT
	cmp	rbp, rax
	je	$LN2@RemoveNonT
	cmp	rbp, rbx
	je	$LN2@RemoveNonT

; 1816 :         if (this != _STD addressof(_Right)) { // splicing from another list, adjust counts

	jmp	SHORT $LN414@RemoveNonT
$LN496@RemoveNonT:

; 1817 : #if _ITERATOR_DEBUG_LEVEL != 0
; 1818 :             if constexpr (!_Alnode_traits::is_always_equal::value) {
; 1819 :                 _STL_VERIFY(_Getal() == _Right._Getal(), "list allocators incompatible for splice");
; 1820 :             }
; 1821 : #endif // _ITERATOR_DEBUG_LEVEL != 0
; 1822 : 
; 1823 :             auto& _My_data = _Mypair._Myval2;
; 1824 :             if (max_size() - _My_data._Mysize < _Count) {

	mov	rcx, 768614336404564650			; 0aaaaaaaaaaaaaaaH
	mov	rdx, QWORD PTR breakableCC$[rsp+8]
	sub	rcx, rdx
	cmp	rcx, 1
	jb	$LN500@RemoveNonT

; 1826 :             }
; 1827 : 
; 1828 :             auto& _Right_data = _Right._Mypair._Myval2;
; 1829 : #if _ITERATOR_DEBUG_LEVEL == 2
; 1830 :             // transfer ownership
; 1831 :             if (_Count == 1) {
; 1832 :                 _My_data._Adopt_unique(_Right_data, _First);
; 1833 :             } else if (_Count == _Right_data._Mysize) {
; 1834 :                 _My_data._Adopt_all(_Right_data);
; 1835 :             } else {
; 1836 :                 _My_data._Adopt_range(_Right_data, _First, _Last);
; 1837 :             }
; 1838 : #endif // _ITERATOR_DEBUG_LEVEL == 2
; 1839 : 
; 1840 :             _My_data._Mysize += _Count;

	inc	rdx
	mov	QWORD PTR breakableCC$[rsp+8], rdx

; 1841 :             _Right_data._Mysize -= _Count;

	dec	QWORD PTR [rsi+8]
$LN414@RemoveNonT:

; 484  :         const auto _First_prev  = _First->_Prev;

	mov	r8, QWORD PTR [rax+8]

; 485  :         _First_prev->_Next      = _Last;

	mov	QWORD PTR [r8], rbx

; 486  :         const auto _Last_prev   = _Last->_Prev;

	mov	rcx, QWORD PTR [rbx+8]

; 487  :         _Last_prev->_Next       = _Before;

	mov	QWORD PTR [rcx], rbp

; 488  :         const auto _Before_prev = _Before->_Prev;

	mov	rdx, QWORD PTR [rbp+8]

; 489  :         _Before_prev->_Next     = _First;

	mov	QWORD PTR [rdx], rax

; 490  : 
; 491  :         // fixup the _Prev values
; 492  :         _Before->_Prev = _Last_prev;

	mov	QWORD PTR [rbp+8], rcx

; 493  :         _Last->_Prev   = _First_prev;

	mov	QWORD PTR [rbx+8], r8

; 494  :         _First->_Prev  = _Before_prev;

	mov	QWORD PTR [rax+8], rdx
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 53   :         }

	jmp	$LN2@RemoveNonT
$LN498@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\source\tests\playerbots\v20_targets\main.cpp

; 101  :         assert(player == bot); ai->world->event(3, target->guid);

	mov	rcx, QWORD PTR [r14]
	mov	r8, QWORD PTR [rdi]
	mov	edx, 3
	mov	rcx, QWORD PTR [rcx]
	call	?event@World@@QEAAXI_K@Z		; World::event

; 102  :         return target->ignoreCalls++ == 0 ? target->state.ignoreFirst : target->state.ignoreSecond;

	mov	ecx, DWORD PTR [rdi+16]
	lea	eax, DWORD PTR [rcx+1]
	mov	DWORD PTR [rdi+16], eax
	test	ecx, ecx
	lea	rax, QWORD PTR [rdi+9]
	je	SHORT $LN227@RemoveNonT
	lea	rax, QWORD PTR [rdi+10]
$LN227@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 54   :         else if (!HasIgnoreCCRti(target, bot) && HasUnBreakableCC(target, bot))

	cmp	BYTE PTR [rax], 0
	jne	$LN9@RemoveNonT
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\source\tests\playerbots\v20_targets\main.cpp

; 108  :         assert(player == bot); ai->world->event(5, target->guid); return target->state.unbreakable;

	mov	rcx, QWORD PTR [r14]
	mov	r8, QWORD PTR [rdi]
	mov	edx, 5
	mov	rcx, QWORD PTR [rcx]
	call	?event@World@@QEAAXI_K@Z		; World::event
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 54   :         else if (!HasIgnoreCCRti(target, bot) && HasUnBreakableCC(target, bot))

	cmp	BYTE PTR [rdi+12], 0
	je	SHORT $LN9@RemoveNonT

; 55   :         {
; 56   :             std::list<ObjectGuid>::iterator tIter2 = tIter;

	mov	rax, rbx
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 164  :         this->_Ptr = this->_Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]

; 1560 :         if (this != _STD addressof(_Right) || (_UWhere != _UFirst && _UWhere != _ULast)) {

	lea	rcx, QWORD PTR unBreakableCC$[rsp]
	cmp	rcx, rsi
	jne	SHORT $LN495@RemoveNonT
	cmp	r15, rax
	je	$LN2@RemoveNonT
	cmp	r15, rbx
	je	$LN2@RemoveNonT

; 1816 :         if (this != _STD addressof(_Right)) { // splicing from another list, adjust counts

	jmp	SHORT $LN456@RemoveNonT
$LN495@RemoveNonT:

; 1817 : #if _ITERATOR_DEBUG_LEVEL != 0
; 1818 :             if constexpr (!_Alnode_traits::is_always_equal::value) {
; 1819 :                 _STL_VERIFY(_Getal() == _Right._Getal(), "list allocators incompatible for splice");
; 1820 :             }
; 1821 : #endif // _ITERATOR_DEBUG_LEVEL != 0
; 1822 : 
; 1823 :             auto& _My_data = _Mypair._Myval2;
; 1824 :             if (max_size() - _My_data._Mysize < _Count) {

	mov	rcx, 768614336404564650			; 0aaaaaaaaaaaaaaaH
	sub	rcx, r13
	cmp	rcx, 1
	jb	$LN500@RemoveNonT

; 1826 :             }
; 1827 : 
; 1828 :             auto& _Right_data = _Right._Mypair._Myval2;
; 1829 : #if _ITERATOR_DEBUG_LEVEL == 2
; 1830 :             // transfer ownership
; 1831 :             if (_Count == 1) {
; 1832 :                 _My_data._Adopt_unique(_Right_data, _First);
; 1833 :             } else if (_Count == _Right_data._Mysize) {
; 1834 :                 _My_data._Adopt_all(_Right_data);
; 1835 :             } else {
; 1836 :                 _My_data._Adopt_range(_Right_data, _First, _Last);
; 1837 :             }
; 1838 : #endif // _ITERATOR_DEBUG_LEVEL == 2
; 1839 : 
; 1840 :             _My_data._Mysize += _Count;

	inc	r13
	mov	QWORD PTR unBreakableCC$[rsp+8], r13

; 1841 :             _Right_data._Mysize -= _Count;

	dec	QWORD PTR [rsi+8]
$LN456@RemoveNonT:

; 484  :         const auto _First_prev  = _First->_Prev;

	mov	r8, QWORD PTR [rax+8]

; 485  :         _First_prev->_Next      = _Last;

	mov	QWORD PTR [r8], rbx

; 486  :         const auto _Last_prev   = _Last->_Prev;

	mov	rcx, QWORD PTR [rbx+8]

; 487  :         _Last_prev->_Next       = _Before;

	mov	QWORD PTR [rcx], r15

; 488  :         const auto _Before_prev = _Before->_Prev;

	mov	rdx, QWORD PTR [r15+8]

; 489  :         _Before_prev->_Next     = _First;

	mov	QWORD PTR [rdx], rax

; 490  : 
; 491  :         // fixup the _Prev values
; 492  :         _Before->_Prev = _Last_prev;

	mov	QWORD PTR [r15+8], rcx

; 493  :         _Last->_Prev   = _First_prev;

	mov	QWORD PTR [rbx+8], r8

; 494  :         _First->_Prev  = _Before_prev;

	mov	QWORD PTR [rax+8], rdx
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 59   :         }

	jmp	SHORT $LN2@RemoveNonT
$LN9@RemoveNonT:

; 60   :         else
; 61   :         {
; 62   :             if (getOne)

	cmp	BYTE PTR getOne$[rsp], 0
	jne	SHORT $LN475@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 164  :         this->_Ptr = this->_Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]
	jmp	SHORT $LN2@RemoveNonT
$LN165@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\source\tests\playerbots\v20_targets\main.cpp

; 97   :         ai->world->event(2, target ? target->guid : 0);

	xor	r8d, r8d
	call	?event@World@@QEAAXI_K@Z		; World::event
$LN167@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 44   :             std::list<ObjectGuid>::iterator tIter2 = tIter;

	mov	rcx, rbx
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 164  :         this->_Ptr = this->_Ptr->_Next;

	mov	rdx, QWORD PTR [rbx]
	mov	rbx, rdx

; 405  :         _Pnode->_Prev->_Next = _Pnode->_Next;

	mov	rax, QWORD PTR [rcx+8]
	mov	QWORD PTR [rax], rdx

; 406  :         _Pnode->_Next->_Prev = _Pnode->_Prev;

	mov	rdx, QWORD PTR [r12]
	mov	rax, QWORD PTR [rcx+8]
	mov	QWORD PTR [rdx+8], rax

; 407  :         --_Mysize;

	dec	QWORD PTR [rsi+8]
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	call	??3@YAXPEAX_K@Z				; operator delete
$LN2@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 202  :         return !(*this == _Right);

	cmp	rbx, QWORD PTR [rsi]
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 39   :     for(std::list<ObjectGuid>::iterator tIter = targets.begin(); tIter != targets.end();)

	jne	$LL4@RemoveNonT
	jmp	SHORT $LN503@RemoveNonT
$LN475@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1105 :         return iterator(_Mypair._Myval2._Myhead->_Next, _STD addressof(_Mypair._Myval2));

	mov	rdx, QWORD PTR [rsi]

; 1450 :         return _Make_iter(_Unchecked_erase(_First._Ptr, _Last._Ptr));

	mov	r8, rbx
	mov	rdx, QWORD PTR [rdx]
	mov	rcx, rsi
	call	?_Unchecked_erase@?$list@_KV?$allocator@_K@std@@@std@@AEAAPEAU?$_List_node@_KPEAX@2@PEAU32@QEAU32@@Z ; std::list<unsigned __int64,std::allocator<unsigned __int64> >::_Unchecked_erase
	mov	r8, QWORD PTR [rsi]
	mov	rdx, QWORD PTR [rbx]
	mov	rcx, rsi
	call	?_Unchecked_erase@?$list@_KV?$allocator@_K@std@@@std@@AEAAPEAU?$_List_node@_KPEAX@2@PEAU32@QEAU32@@Z ; std::list<unsigned __int64,std::allocator<unsigned __int64> >::_Unchecked_erase
$LN503@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 77   :     if (targets.empty())

	mov	rdx, 768614336404564650			; 0aaaaaaaaaaaaaaaH
	mov	rdi, QWORD PTR breakableCC$[rsp+8]
$LN482@RemoveNonT:
	cmp	QWORD PTR [rsi+8], 0
	jne	$LN340@RemoveNonT

; 78   :     {
; 79   :         if (!unBreakableCC.empty())

	test	r13, r13
	je	SHORT $LN14@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1113 :         return iterator(_Mypair._Myval2._Myhead, _STD addressof(_Mypair._Myval2));

	mov	r9, QWORD PTR [rsi]

; 1532 :         if (this != _STD addressof(_Right) && _Right_data._Mysize != 0) { // worth splicing, do it

	lea	rax, QWORD PTR unBreakableCC$[rsp]
	cmp	rsi, rax
	je	SHORT $LN340@RemoveNonT

; 1533 : #if _ITERATOR_DEBUG_LEVEL == 2
; 1534 :             _STL_VERIFY(_Where._Getcont() == _STD addressof(_Mypair._Myval2), "list splice iterator outside range");
; 1535 : #endif // _ITERATOR_DEBUG_LEVEL == 2
; 1536 :             const auto _Right_head = _Right_data._Myhead;
; 1537 :             _Splice(_Where._Ptr, _Right, _Right_head->_Next, _Right_head, _Right_data._Mysize);

	mov	r8, QWORD PTR [r15]

; 1824 :             if (max_size() - _My_data._Mysize < _Count) {

	cmp	r13, rdx
	ja	$LN500@RemoveNonT

; 1826 :             }
; 1827 : 
; 1828 :             auto& _Right_data = _Right._Mypair._Myval2;
; 1829 : #if _ITERATOR_DEBUG_LEVEL == 2
; 1830 :             // transfer ownership
; 1831 :             if (_Count == 1) {
; 1832 :                 _My_data._Adopt_unique(_Right_data, _First);
; 1833 :             } else if (_Count == _Right_data._Mysize) {
; 1834 :                 _My_data._Adopt_all(_Right_data);
; 1835 :             } else {
; 1836 :                 _My_data._Adopt_range(_Right_data, _First, _Last);
; 1837 :             }
; 1838 : #endif // _ITERATOR_DEBUG_LEVEL == 2
; 1839 : 
; 1840 :             _My_data._Mysize += _Count;

	mov	QWORD PTR [rsi+8], r13

; 484  :         const auto _First_prev  = _First->_Prev;

	mov	rdx, QWORD PTR [r8+8]

; 485  :         _First_prev->_Next      = _Last;

	mov	QWORD PTR [rdx], r15

; 486  :         const auto _Last_prev   = _Last->_Prev;

	mov	rcx, QWORD PTR [r15+8]

; 487  :         _Last_prev->_Next       = _Before;

	mov	QWORD PTR [rcx], r9

; 488  :         const auto _Before_prev = _Before->_Prev;

	mov	rax, QWORD PTR [r9+8]

; 489  :         _Before_prev->_Next     = _First;

	mov	QWORD PTR [rax], r8

; 490  : 
; 491  :         // fixup the _Prev values
; 492  :         _Before->_Prev = _Last_prev;

	mov	QWORD PTR [r9+8], rcx

; 493  :         _Last->_Prev   = _First_prev;

	mov	QWORD PTR [r15+8], rdx
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 82   :         }

	jmp	SHORT $LN505@RemoveNonT
$LN14@RemoveNonT:

; 83   :         else if(!breakableCC.empty())

	test	rdi, rdi
	je	SHORT $LN340@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1113 :         return iterator(_Mypair._Myval2._Myhead, _STD addressof(_Mypair._Myval2));

	mov	r9, QWORD PTR [rsi]

; 1532 :         if (this != _STD addressof(_Right) && _Right_data._Mysize != 0) { // worth splicing, do it

	lea	rax, QWORD PTR breakableCC$[rsp]
	cmp	rsi, rax
	je	SHORT $LN340@RemoveNonT

; 1533 : #if _ITERATOR_DEBUG_LEVEL == 2
; 1534 :             _STL_VERIFY(_Where._Getcont() == _STD addressof(_Mypair._Myval2), "list splice iterator outside range");
; 1535 : #endif // _ITERATOR_DEBUG_LEVEL == 2
; 1536 :             const auto _Right_head = _Right_data._Myhead;
; 1537 :             _Splice(_Where._Ptr, _Right, _Right_head->_Next, _Right_head, _Right_data._Mysize);

	mov	r8, QWORD PTR [rbp]

; 1824 :             if (max_size() - _My_data._Mysize < _Count) {

	cmp	rdi, rdx
	ja	$LN500@RemoveNonT

; 1826 :             }
; 1827 : 
; 1828 :             auto& _Right_data = _Right._Mypair._Myval2;
; 1829 : #if _ITERATOR_DEBUG_LEVEL == 2
; 1830 :             // transfer ownership
; 1831 :             if (_Count == 1) {
; 1832 :                 _My_data._Adopt_unique(_Right_data, _First);
; 1833 :             } else if (_Count == _Right_data._Mysize) {
; 1834 :                 _My_data._Adopt_all(_Right_data);
; 1835 :             } else {
; 1836 :                 _My_data._Adopt_range(_Right_data, _First, _Last);
; 1837 :             }
; 1838 : #endif // _ITERATOR_DEBUG_LEVEL == 2
; 1839 : 
; 1840 :             _My_data._Mysize += _Count;

	mov	QWORD PTR [rsi+8], rdi

; 484  :         const auto _First_prev  = _First->_Prev;

	mov	rdx, QWORD PTR [r8+8]

; 485  :         _First_prev->_Next      = _Last;

	mov	QWORD PTR [rdx], rbp

; 486  :         const auto _Last_prev   = _Last->_Prev;

	mov	rcx, QWORD PTR [rbp+8]

; 487  :         _Last_prev->_Next       = _Before;

	mov	QWORD PTR [rcx], r9

; 488  :         const auto _Before_prev = _Before->_Prev;

	mov	rax, QWORD PTR [r9+8]

; 489  :         _Before_prev->_Next     = _First;

	mov	QWORD PTR [rax], r8

; 490  : 
; 491  :         // fixup the _Prev values
; 492  :         _Before->_Prev = _Last_prev;

	mov	QWORD PTR [r9+8], rcx

; 493  :         _Last->_Prev   = _First_prev;

	mov	QWORD PTR [rbp+8], rdx
$LN505@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 88   : }

	mov	QWORD PTR [r8+8], rax
$LN340@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 324  :         _Head->_Prev->_Next = nullptr;

	mov	rax, QWORD PTR [r15+8]
	mov	QWORD PTR [rax], 0

; 325  : 
; 326  :         auto _Pnode = _Head->_Next;

	mov	rcx, QWORD PTR [r15]

; 327  :         for (_Nodeptr _Pnext; _Pnode; _Pnode = _Pnext) {

	test	rcx, rcx
	je	SHORT $LN483@RemoveNonT
	npad	7
$LL86@RemoveNonT:

; 328  :             _Pnext = _Pnode->_Next;

	mov	rbx, QWORD PTR [rcx]
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	call	??3@YAXPEAX_K@Z				; operator delete
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 327  :         for (_Nodeptr _Pnext; _Pnode; _Pnode = _Pnext) {

	mov	rcx, rbx
	test	rbx, rbx
	jne	SHORT $LL86@RemoveNonT
$LN483@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	mov	rcx, r15
	call	??3@YAXPEAX_K@Z				; operator delete
	npad	1
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 324  :         _Head->_Prev->_Next = nullptr;

	mov	rax, QWORD PTR [rbp+8]
	mov	QWORD PTR [rax], 0

; 325  : 
; 326  :         auto _Pnode = _Head->_Next;

	mov	rcx, QWORD PTR [rbp]

; 327  :         for (_Nodeptr _Pnext; _Pnode; _Pnode = _Pnext) {

	test	rcx, rcx
	je	SHORT $LN484@RemoveNonT
	npad	9
$LL29@RemoveNonT:

; 328  :             _Pnext = _Pnode->_Next;

	mov	rbx, QWORD PTR [rcx]
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	call	??3@YAXPEAX_K@Z				; operator delete
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 327  :         for (_Nodeptr _Pnext; _Pnode; _Pnode = _Pnext) {

	mov	rcx, rbx
	test	rbx, rbx
	jne	SHORT $LL29@RemoveNonT
$LN484@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	mov	rcx, rbp
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v20.inc

; 88   : }

	mov	rbx, QWORD PTR [rsp+152]
	movaps	xmm6, XMMWORD PTR [rsp+64]
	add	rsp, 80					; 00000050H
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	rdi
	pop	rsi
	pop	rbp
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	jmp	??3@YAXPEAX_K@Z				; operator delete
$LN500@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1825 :                 _Xlength_error("list too long");

	lea	rcx, OFFSET FLAT:??_C@_0O@NKNMEGII@list?5too?5long@
	call	?_Xlength_error@std@@YAXPEBD@Z		; std::_Xlength_error
	int	3
$LN504@RemoveNonT:
?RemoveNonThreating@PossibleAttackTargetsValue@v20@@QEAAXAEAV?$list@_KV?$allocator@_K@std@@@std@@_N@Z ENDP ; v20::PossibleAttackTargetsValue::RemoveNonThreating