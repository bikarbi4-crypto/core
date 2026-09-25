?RemoveNonThreating@PossibleAttackTargetsValue@v19@@QEAAXAEAV?$list@_KV?$allocator@_K@std@@@std@@_N@Z PROC ; v19::PossibleAttackTargetsValue::RemoveNonThreating, COMDAT

; 35   : {

$LN1015:
	mov	rax, rsp
	mov	QWORD PTR [rax+8], rbx
	mov	QWORD PTR [rax+24], rsi
	mov	QWORD PTR [rax+32], rdi
	push	rbp
	push	r12
	push	r13
	push	r14
	push	r15
	lea	rbp, QWORD PTR [rax-95]
	sub	rsp, 144				; 00000090H
	movaps	XMMWORD PTR [rax-56], xmm6
	movzx	r13d, r8b
	mov	r15, rdx
	mov	r14, rcx
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 353  :     _List_val() noexcept : _Myhead(), _Mysize(0) {} // initialize data

	xor	ebx, ebx
	mov	QWORD PTR breakableCC$[rbp-89], rbx
	mov	QWORD PTR breakableCC$[rbp-81], rbx
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1852 :         _Construct_in_place(_Newhead->_Next, _Newhead);

	mov	QWORD PTR [rax], rax

; 1853 :         _Construct_in_place(_Newhead->_Prev, _Newhead);

	mov	QWORD PTR [rax+8], rax

; 1854 :         _Mypair._Myval2._Myhead = _Newhead;

	mov	QWORD PTR breakableCC$[rbp-89], rax

; 353  :     _List_val() noexcept : _Myhead(), _Mysize(0) {} // initialize data

	mov	QWORD PTR unBreakableCC$[rbp-89], rbx
	mov	QWORD PTR unBreakableCC$[rbp-81], rbx
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1852 :         _Construct_in_place(_Newhead->_Next, _Newhead);

	mov	QWORD PTR [rax], rax

; 1853 :         _Construct_in_place(_Newhead->_Prev, _Newhead);

	mov	QWORD PTR [rax+8], rax

; 1854 :         _Mypair._Myval2._Myhead = _Newhead;

	mov	QWORD PTR unBreakableCC$[rbp-89], rax

; 1105 :         return iterator(_Mypair._Myval2._Myhead->_Next, _STD addressof(_Mypair._Myval2));

	mov	rax, QWORD PTR [r15]

; 37   :     _List_unchecked_const_iterator(_Nodeptr _Pnode, const _Mylist* _Plist) noexcept : _Ptr(_Pnode) {

	mov	rbx, QWORD PTR [rax]

; 202  :         return !(*this == _Right);

	cmp	rbx, rax
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 39   :     for(std::list<ObjectGuid>::iterator tIter = targets.begin(); tIter != targets.end();)

	je	$LN1010@RemoveNonT
	movss	xmm6, DWORD PTR __real@42700000
	npad	12
$LL4@RemoveNonT:

; 40   :     {
; 41   :         Unit* target = ai->GetUnit(*tIter);

	mov	r12, rbx
	mov	rsi, QWORD PTR [r14]
	mov	rdi, QWORD PTR [rbx+16]
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\source\tests\playerbots\v20_targets\main.cpp

; 79   :         world->event(1, guid);

	mov	r8, rdi
	mov	edx, 1
	mov	rcx, QWORD PTR [rsi]
	call	?event@World@@QEAAXI_K@Z		; World::event

; 80   :         if (!guid) return nullptr;

	test	rdi, rdi
	jne	SHORT $LN156@RemoveNonT
	xor	esi, esi
	mov	edi, esi
	jmp	SHORT $LN155@RemoveNonT
$LN156@RemoveNonT:

; 81   :         assert(guid <= world->units.size());

	cmp	rdi, 32					; 00000020H
	jbe	SHORT $LN158@RemoveNonT
	mov	r8d, 81					; 00000051H
	lea	rdx, OFFSET FLAT:??_C@_1BBE@NHKIELNI@?$AAC?$AA?3?$AA?2?$AAU?$AAs?$AAe?$AAr?$AAs?$AA?2?$AAu?$AAs?$AAe?$AAr?$AA?2?$AAD@
	lea	rcx, OFFSET FLAT:??_C@_1DI@ELKMIOFL@?$AAg?$AAu?$AAi?$AAd?$AA?5?$AA?$DM?$AA?$DN?$AA?5?$AAw?$AAo?$AAr?$AAl?$AAd?$AA?9?$AA?$DO@
	call	QWORD PTR __imp__wassert
$LN158@RemoveNonT:

; 82   :         Unit* unit = &world->units[guid - 1];

	lea	rdi, QWORD PTR [rdi+rdi*2]
	mov	rax, QWORD PTR [rsi]
	lea	rdi, QWORD PTR [rdi-3]
	lea	rdi, QWORD PTR [rax+rdi*8]

; 83   :         unit->ignoreCalls = 0;

	xor	esi, esi
	mov	DWORD PTR [rdi+16], esi
$LN155@RemoveNonT:

; 96   :         assert(player == bot && range == 60.0f && ignoreCC && !checkAttackerValid);

	movss	xmm0, DWORD PTR ?sPlayerbotAIConfig@@3UConfig@@A
	ucomiss	xmm0, xmm6
	jp	SHORT $LN1004@RemoveNonT
	je	SHORT $LN165@RemoveNonT
$LN1004@RemoveNonT:
	mov	r8d, 96					; 00000060H
	lea	rdx, OFFSET FLAT:??_C@_1BBE@NHKIELNI@?$AAC?$AA?3?$AA?2?$AAU?$AAs?$AAe?$AAr?$AAs?$AA?2?$AAu?$AAs?$AAe?$AAr?$AA?2?$AAD@
	lea	rcx, OFFSET FLAT:??_C@_1IG@PLCGPAAI@?$AAp?$AAl?$AAa?$AAy?$AAe?$AAr?$AA?5?$AA?$DN?$AA?$DN?$AA?5?$AAb?$AAo?$AAt?$AA?5?$AA?$CG@
	call	QWORD PTR __imp__wassert
$LN165@RemoveNonT:

; 97   :         ai->world->event(2, target ? target->guid : 0);

	mov	rax, QWORD PTR [r14]
	mov	rcx, QWORD PTR [rax]
	mov	edx, 2
	test	rdi, rdi
	je	$LN166@RemoveNonT
	mov	r8, QWORD PTR [rdi]
	call	?event@World@@QEAAXI_K@Z		; World::event

; 98   :         return target && target->state.valid;

	cmp	BYTE PTR [rdi+8], 0
	je	$LN168@RemoveNonT

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
	je	SHORT $LN223@RemoveNonT
	lea	rax, QWORD PTR [rdi+10]
$LN223@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 48   :         else if (!HasIgnoreCCRti(target, bot) && HasBreakableCC(target, bot))

	cmp	BYTE PTR [rax], 0
	jne	SHORT $LN1001@RemoveNonT
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\source\tests\playerbots\v20_targets\main.cpp

; 105  :         assert(player == bot); ai->world->event(4, target->guid); return target->state.breakable;

	mov	rcx, QWORD PTR [r14]
	mov	r8, QWORD PTR [rdi]
	mov	edx, 4
	mov	rcx, QWORD PTR [rcx]
	call	?event@World@@QEAAXI_K@Z		; World::event
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 48   :         else if (!HasIgnoreCCRti(target, bot) && HasBreakableCC(target, bot))

	cmp	BYTE PTR [rdi+11], 0
	je	SHORT $LN1001@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1274 :         _Emplace(_Mypair._Myval2._Myhead, _Val);

	mov	rdi, QWORD PTR breakableCC$[rbp-89]

; 1030 :         if (_Mysize == max_size()) {

	mov	rax, 768614336404564650			; 0aaaaaaaaaaaaaaaH
	cmp	QWORD PTR breakableCC$[rbp-81], rax
	je	$LN939@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 1160 :     _CONSTEXPR20 explicit _Alloc_construct_ptr(_Alloc& _Al_) : _Al(_Al_), _Ptr(nullptr) {}

	lea	rax, QWORD PTR breakableCC$[rbp-89]
	mov	QWORD PTR _Op$5[rbp-89], rax

; 1167 :         _Ptr = nullptr; // if allocate throws, prevents double-free

	mov	QWORD PTR _Op$5[rbp-81], rsi

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 595  :         _Alnode_traits::construct(this->_Al, _STD addressof(this->_Ptr->_Myval), _STD forward<_Valtys>(_Vals)...);

	mov	rcx, QWORD PTR [rbx+16]
	mov	QWORD PTR [rax+16], rcx

; 1035 :         ++_Mysize;

	inc	QWORD PTR breakableCC$[rbp-81]

; 608  :         const pointer _Insert_after = _Insert_before->_Prev;

	mov	rcx, QWORD PTR [rdi+8]

; 609  :         _Construct_in_place(this->_Ptr->_Next, _Insert_before);

	mov	QWORD PTR [rax], rdi

; 610  :         _Construct_in_place(this->_Ptr->_Prev, _Insert_after);

	mov	QWORD PTR [rax+8], rcx

; 611  :         const auto _Result    = this->_Ptr;
; 612  :         this->_Ptr            = pointer{};

	mov	QWORD PTR _Op$5[rbp-81], rsi

; 613  :         _Insert_before->_Prev = _Result;

	mov	QWORD PTR [rdi+8], rax

; 614  :         _Insert_after->_Next  = _Result;

	mov	QWORD PTR [rcx], rax
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 54   :         }

	jmp	$LN168@RemoveNonT
$LN1001@RemoveNonT:
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
	je	SHORT $LN228@RemoveNonT
	lea	rax, QWORD PTR [rdi+10]
$LN228@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 55   :         else if (!HasIgnoreCCRti(target, bot) && HasUnBreakableCC(target, bot))

	cmp	BYTE PTR [rax], 0
	jne	SHORT $LN9@RemoveNonT
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\source\tests\playerbots\v20_targets\main.cpp

; 108  :         assert(player == bot); ai->world->event(5, target->guid); return target->state.unbreakable;

	mov	rcx, QWORD PTR [r14]
	mov	r8, QWORD PTR [rdi]
	mov	edx, 5
	mov	rcx, QWORD PTR [rcx]
	call	?event@World@@QEAAXI_K@Z		; World::event
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 55   :         else if (!HasIgnoreCCRti(target, bot) && HasUnBreakableCC(target, bot))

	cmp	BYTE PTR [rdi+12], 0
	je	SHORT $LN9@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1274 :         _Emplace(_Mypair._Myval2._Myhead, _Val);

	mov	rdi, QWORD PTR unBreakableCC$[rbp-89]

; 1030 :         if (_Mysize == max_size()) {

	mov	rax, 768614336404564650			; 0aaaaaaaaaaaaaaaH
	cmp	QWORD PTR unBreakableCC$[rbp-81], rax
	je	$LN939@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 1160 :     _CONSTEXPR20 explicit _Alloc_construct_ptr(_Alloc& _Al_) : _Al(_Al_), _Ptr(nullptr) {}

	lea	rax, QWORD PTR unBreakableCC$[rbp-89]
	mov	QWORD PTR _Op$8[rbp-89], rax

; 1167 :         _Ptr = nullptr; // if allocate throws, prevents double-free

	mov	QWORD PTR _Op$8[rbp-81], rsi

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 595  :         _Alnode_traits::construct(this->_Al, _STD addressof(this->_Ptr->_Myval), _STD forward<_Valtys>(_Vals)...);

	mov	rcx, QWORD PTR [rbx+16]
	mov	QWORD PTR [rax+16], rcx

; 1035 :         ++_Mysize;

	inc	QWORD PTR unBreakableCC$[rbp-81]

; 608  :         const pointer _Insert_after = _Insert_before->_Prev;

	mov	rcx, QWORD PTR [rdi+8]

; 609  :         _Construct_in_place(this->_Ptr->_Next, _Insert_before);

	mov	QWORD PTR [rax], rdi

; 610  :         _Construct_in_place(this->_Ptr->_Prev, _Insert_after);

	mov	QWORD PTR [rax+8], rcx

; 611  :         const auto _Result    = this->_Ptr;
; 612  :         this->_Ptr            = pointer{};

	mov	QWORD PTR _Op$8[rbp-81], rsi

; 613  :         _Insert_before->_Prev = _Result;

	mov	QWORD PTR [rdi+8], rax

; 614  :         _Insert_after->_Next  = _Result;

	mov	QWORD PTR [rcx], rax
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 61   :         }

	jmp	SHORT $LN168@RemoveNonT
$LN9@RemoveNonT:

; 62   :         else
; 63   :         {
; 64   :             if (getOne)

	test	r13b, r13b
	jne	SHORT $LN942@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 164  :         this->_Ptr = this->_Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]
	jmp	SHORT $LN2@RemoveNonT
$LN166@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\source\tests\playerbots\v20_targets\main.cpp

; 97   :         ai->world->event(2, target ? target->guid : 0);

	xor	r8d, r8d
	call	?event@World@@QEAAXI_K@Z		; World::event
$LN168@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 202  :         return !(*this == _Right);

	mov	rcx, rbx
	mov	rdx, QWORD PTR [rbx]
	mov	rax, QWORD PTR [rbx+8]
	mov	rbx, rdx
	mov	QWORD PTR [rax], rdx
	mov	rdx, QWORD PTR [r12]
	mov	rax, QWORD PTR [rcx+8]
	mov	QWORD PTR [rdx+8], rax
	dec	QWORD PTR [r15+8]
	mov	edx, 24
	call	??3@YAXPEAX_K@Z				; operator delete
$LN2@RemoveNonT:
	cmp	rbx, QWORD PTR [r15]
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 39   :     for(std::list<ObjectGuid>::iterator tIter = targets.begin(); tIter != targets.end();)

	jne	$LL4@RemoveNonT
$LN1010@RemoveNonT:

; 70   :             }
; 71   :             else
; 72   :             {
; 73   :                 ++tIter;
; 74   :             }
; 75   :         }
; 76   :     }
; 77   : 
; 78   :     if (targets.empty())

	xor	r13d, r13d
$LN1006@RemoveNonT:
	cmp	QWORD PTR [r15+8], 0
	jne	$LN423@RemoveNonT

; 79   :     {
; 80   :         if (!unBreakableCC.empty())

	cmp	QWORD PTR unBreakableCC$[rbp-81], 0
	je	$LN14@RemoveNonT

; 81   :         {
; 82   :             targets = unBreakableCC;

	lea	rdx, QWORD PTR unBreakableCC$[rbp-89]
	mov	rcx, r15
	call	??4?$list@_KV?$allocator@_K@std@@@std@@QEAAAEAV01@AEBV01@@Z ; std::list<unsigned __int64,std::allocator<unsigned __int64> >::operator=

; 83   :         }

	jmp	$LN423@RemoveNonT
$LN942@RemoveNonT:

; 65   :             {
; 66   :                 // If the target is valid return it straight away
; 67   :                 std::list<ObjectGuid> result = { *tIter };

	mov	rax, QWORD PTR [rbx+16]
	mov	QWORD PTR $T9[rbp-89], rax
	xorps	xmm0, xmm0
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 353  :     _List_val() noexcept : _Myhead(), _Mysize(0) {} // initialize data

	movdqu	XMMWORD PTR result$2[rbp-89], xmm0

; 1040 :         _Construct_range_unchecked(_Ilist.begin(), _Ilist.end());

	lea	r8, QWORD PTR $T9[rbp-81]
	lea	rdx, QWORD PTR $T9[rbp-89]
	lea	rcx, QWORD PTR result$2[rbp-89]
	call	??$_Construct_range_unchecked@PEB_KPEB_K@?$list@_KV?$allocator@_K@std@@@std@@AEAAXPEB_KQEB_K@Z ; std::list<unsigned __int64,std::allocator<unsigned __int64> >::_Construct_range_unchecked<unsigned __int64 const *,unsigned __int64 const *>
	npad	1

; 1086 :         if (this == _STD addressof(_Right)) {

	lea	rax, QWORD PTR result$2[rbp-89]
	cmp	r15, rax
	je	SHORT $LN1011@RemoveNonT

; 37   :     _List_unchecked_const_iterator(_Nodeptr _Pnode, const _Mylist* _Plist) noexcept : _Ptr(_Pnode) {

	mov	r12, QWORD PTR result$2[rbp-89]
	mov	rbx, QWORD PTR [r12]

; 1324 :         const auto _Myend = _Mypair._Myval2._Myhead;

	mov	r8, QWORD PTR [r15]

; 1325 :         auto _Old         = _Myend->_Next;

	mov	rdx, QWORD PTR [r8]
	cmp	rbx, r12

; 1326 :         for (;;) { // attempt to reuse a node
; 1327 :             if (_First == _Last) {

	je	SHORT $LN943@RemoveNonT
$LL497@RemoveNonT:

; 1330 :                 return;
; 1331 :             }
; 1332 : 
; 1333 :             if (_Old == _Myend) { // no more nodes to reuse, append the rest

	cmp	rdx, r8
	je	SHORT $LN944@RemoveNonT

; 1334 :                 _List_node_insert_op2<_Alnode> _Op(_Getal());
; 1335 :                 _Op._Append_range_unchecked(_STD move(_First), _Last);
; 1336 :                 _Op._Attach_at_end(_Mypair._Myval2);
; 1337 :                 return;
; 1338 :             }
; 1339 : 
; 1340 :             // reuse the node
; 1341 :             _Old->_Myval = *_First;

	mov	rax, QWORD PTR [rbx+16]
	mov	QWORD PTR [rdx+16], rax

; 1342 :             _Old         = _Old->_Next;

	mov	rdx, QWORD PTR [rdx]

; 50   :         _Ptr = _Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]

; 72   :         return _Ptr == _Right._Ptr;

	cmp	rbx, r12

; 1327 :             if (_First == _Last) {

	jne	SHORT $LL497@RemoveNonT
$LN943@RemoveNonT:

; 1328 :                 // input sequence exhausted; destroy and deallocate any tail of unneeded nodes
; 1329 :                 _Unchecked_erase(_Old, _Myend);

	mov	rcx, r15
	call	?_Unchecked_erase@?$list@_KV?$allocator@_K@std@@@std@@AEAAPEAU?$_List_node@_KPEAX@2@PEAU32@QEAU32@@Z ; std::list<unsigned __int64,std::allocator<unsigned __int64> >::_Unchecked_erase
$LN1011@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 69   :                 break;

	xor	r13d, r13d
$LN602@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1518 :         _Node::_Free_non_head(_Al, _My_data._Myhead);

	mov	rcx, QWORD PTR result$2[rbp-89]

; 324  :         _Head->_Prev->_Next = nullptr;

	mov	rax, QWORD PTR [rcx+8]
	mov	QWORD PTR [rax], r13

; 325  : 
; 326  :         auto _Pnode = _Head->_Next;

	mov	rcx, QWORD PTR [rcx]

; 327  :         for (_Nodeptr _Pnext; _Pnode; _Pnode = _Pnext) {

	test	rcx, rcx
	je	SHORT $LN965@RemoveNonT
	npad	2
$LL648@RemoveNonT:

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
	jne	SHORT $LL648@RemoveNonT
$LN965@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	mov	rcx, QWORD PTR result$2[rbp-89]
	call	??3@YAXPEAX_K@Z				; operator delete
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 39   :     for(std::list<ObjectGuid>::iterator tIter = targets.begin(); tIter != targets.end();)

	jmp	$LN1006@RemoveNonT
$LN944@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 627  :     explicit _List_node_insert_op2(_Alnode& _Al_) : _Al(_Al_), _Added(0) {}

	mov	QWORD PTR _Op$7[rbp-89], r15
	xor	r13d, r13d
	mov	r14d, r13d
	mov	QWORD PTR _Op$7[rbp-81], r13
	xorps	xmm0, xmm0

; 750  :             value_type::_Freenode(_Al, _STD exchange(_Subject, _Subject->_Next));
; 751  :         }
; 752  :     }
; 753  : 
; 754  : private:
; 755  :     _Alnode& _Al;
; 756  :     size_type _Added; // if 0, the values of _Head and _Tail are indeterminate
; 757  :     pointer _Tail{}; // points to the most recently appended element; it doesn't have _Next constructed

	movdqu	XMMWORD PTR _Op$7[rbp-73], xmm0

; 664  :         if (_First == _Last) { // throws

	cmp	rbx, r12
	je	SHORT $LN1005@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 1160 :     _CONSTEXPR20 explicit _Alloc_construct_ptr(_Alloc& _Al_) : _Al(_Al_), _Ptr(nullptr) {}

	mov	QWORD PTR _Newnode$4[rbp-89], r15

; 1167 :         _Ptr = nullptr; // if allocate throws, prevents double-free

	mov	QWORD PTR _Newnode$4[rbp-81], r13

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
	mov	rdi, rax
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 671  :             _Alnode_traits::construct(_Al, _STD addressof(_Newnode._Ptr->_Myval), *_First); // throws

	mov	rcx, QWORD PTR [rbx+16]
	mov	QWORD PTR [rax+16], rcx
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\utility

; 774  :     _Val         = static_cast<_Other&&>(_New_val);

	mov	QWORD PTR _Newnode$4[rbp-81], r13
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 673  :             _Head               = _Newhead;

	mov	QWORD PTR _Op$7[rbp-65], rax

; 674  :             _Tail               = _Newhead;

	mov	rsi, rax
	mov	QWORD PTR _Op$7[rbp-73], rax

; 675  :             ++_Added;

	mov	r14d, 1
	mov	QWORD PTR _Op$7[rbp-81], r14

; 50   :         _Ptr = _Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]

; 77   :         return !(*this == _Right);

	cmp	rbx, r12

; 679  :         while (_First != _Last) { // throws

	je	SHORT $LN509@RemoveNonT
$LL508@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 1167 :         _Ptr = nullptr; // if allocate throws, prevents double-free

	mov	QWORD PTR _Newnode$4[rbp-81], r13

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 681  :             _Alnode_traits::construct(_Al, _STD addressof(_Newnode._Ptr->_Myval), *_First); // throws

	mov	rcx, QWORD PTR [rbx+16]
	mov	QWORD PTR [rax+16], rcx

; 682  :             _Construct_in_place(_Tail->_Next, _Newnode._Ptr);

	mov	QWORD PTR [rsi], rax

; 683  :             _Construct_in_place(_Newnode._Ptr->_Prev, _Tail);

	mov	QWORD PTR [rax+8], rsi
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\utility

; 774  :     _Val         = static_cast<_Other&&>(_New_val);

	mov	QWORD PTR _Newnode$4[rbp-81], r13
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 684  :             _Tail = _STD exchange(_Newnode._Ptr, pointer{});

	mov	rsi, rax
	mov	QWORD PTR _Op$7[rbp-73], rax

; 685  :             ++_Added;

	inc	r14
	mov	QWORD PTR _Op$7[rbp-81], r14

; 50   :         _Ptr = _Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]

; 77   :         return !(*this == _Right);

	cmp	rbx, r12

; 679  :         while (_First != _Last) { // throws

	jne	SHORT $LL508@RemoveNonT
$LN509@RemoveNonT:

; 628  : 
; 629  :     _List_node_insert_op2(const _List_node_insert_op2&)            = delete;
; 630  :     _List_node_insert_op2& operator=(const _List_node_insert_op2&) = delete;
; 631  : 
; 632  :     template <class... _CArgT>
; 633  :     void _Append_n(size_type _Count, const _CArgT&... _Carg) {
; 634  :         // Append _Count elements constructed from _Carg
; 635  :         if (_Count <= 0) {
; 636  :             return;
; 637  :         }
; 638  : 
; 639  :         _Alloc_construct_ptr<_Alnode> _Newnode(_Al);
; 640  :         if (_Added == 0) {
; 641  :             _Newnode._Allocate(); // throws
; 642  :             _Alnode_traits::construct(_Al, _STD addressof(_Newnode._Ptr->_Myval), _Carg...); // throws
; 643  :             _Head = _Newnode._Ptr;
; 644  :             _Tail = _Newnode._Ptr;
; 645  :             ++_Added;
; 646  :             --_Count;
; 647  :         }
; 648  : 
; 649  :         for (; 0 < _Count; --_Count) {
; 650  :             _Newnode._Allocate(); // throws
; 651  :             _Alnode_traits::construct(_Al, _STD addressof(_Newnode._Ptr->_Myval), _Carg...); // throws
; 652  :             _Construct_in_place(_Tail->_Next, _Newnode._Ptr);
; 653  :             _Construct_in_place(_Newnode._Ptr->_Prev, _Tail);
; 654  :             _Tail = _Newnode._Ptr;
; 655  :             ++_Added;
; 656  :         }
; 657  : 
; 658  :         _Newnode._Ptr = pointer{};
; 659  :     }
; 660  : 
; 661  :     template <class _InIt, class _Sentinel>
; 662  :     void _Append_range_unchecked(_InIt _First, const _Sentinel _Last) {
; 663  :         // Append the values in [_First, _Last)
; 664  :         if (_First == _Last) { // throws

	jmp	SHORT $LN559@RemoveNonT
$LN1005@RemoveNonT:
	mov	rdi, QWORD PTR _Op$7[rbp-65]
	mov	rsi, QWORD PTR _Op$7[rbp-73]
$LN559@RemoveNonT:

; 717  :         _Attach_before(_List_data, _List_data._Myhead);

	mov	rcx, QWORD PTR [r15]

; 697  :         if (_Local_added == 0) {

	test	r14, r14
	je	SHORT $LN1002@RemoveNonT

; 698  :             return _Insert_before;
; 699  :         }
; 700  : 
; 701  :         const auto _Local_head   = _Head;
; 702  :         const auto _Local_tail   = _Tail;
; 703  :         const auto _Insert_after = _Insert_before->_Prev;

	mov	rax, QWORD PTR [rcx+8]

; 704  : 
; 705  :         _Construct_in_place(_Local_head->_Prev, _Insert_after);

	mov	QWORD PTR [rdi+8], rax

; 706  :         _Insert_after->_Next = _Local_head;

	mov	QWORD PTR [rax], rdi

; 707  :         _Construct_in_place(_Local_tail->_Next, _Insert_before);

	mov	QWORD PTR [rsi], rcx

; 708  :         _Insert_before->_Prev = _Local_tail;

	mov	QWORD PTR [rcx+8], rsi

; 709  : 
; 710  :         _List_data._Mysize += _Local_added;

	add	QWORD PTR [r15+8], r14

; 711  :         _Added = 0;

	mov	r14, r13
$LN1002@RemoveNonT:

; 742  :         if (_Added == 0) {

	test	r14, r14
	je	$LN602@RemoveNonT

; 743  :             return;
; 744  :         }
; 745  : 
; 746  :         _Construct_in_place(_Head->_Prev, pointer{});

	mov	QWORD PTR [rdi+8], r13

; 747  :         _Construct_in_place(_Tail->_Next, pointer{});

	mov	QWORD PTR [rsi], r13
	npad	13
$LL601@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\utility

; 773  :     _Ty _Old_val = static_cast<_Ty&&>(_Val);

	mov	rcx, rdi

; 774  :     _Val         = static_cast<_Other&&>(_New_val);

	mov	rdi, QWORD PTR [rdi]
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	call	??3@YAXPEAX_K@Z				; operator delete
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 749  :         while (_Subject) {

	test	rdi, rdi
	jne	SHORT $LL601@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 219  :     _STL_VERIFY(_Back_shift >= _Min_back_shift && _Back_shift <= _Non_user_size, "invalid argument");

	jmp	$LN602@RemoveNonT
$LN14@RemoveNonT:
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 84   :         else if(!breakableCC.empty())

	cmp	QWORD PTR breakableCC$[rbp-81], 0
	je	SHORT $LN423@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1086 :         if (this == _STD addressof(_Right)) {

	lea	rax, QWORD PTR breakableCC$[rbp-89]
	cmp	r15, rax
	je	SHORT $LN423@RemoveNonT

; 37   :     _List_unchecked_const_iterator(_Nodeptr _Pnode, const _Mylist* _Plist) noexcept : _Ptr(_Pnode) {

	mov	r12, QWORD PTR breakableCC$[rbp-89]
	mov	rbx, QWORD PTR [r12]

; 1324 :         const auto _Myend = _Mypair._Myval2._Myhead;

	mov	r8, QWORD PTR [r15]

; 1325 :         auto _Old         = _Myend->_Next;

	mov	rdx, QWORD PTR [r8]
	cmp	rbx, r12

; 1326 :         for (;;) { // attempt to reuse a node
; 1327 :             if (_First == _Last) {

	je	SHORT $LN947@RemoveNonT
	npad	3
$LL318@RemoveNonT:

; 1330 :                 return;
; 1331 :             }
; 1332 : 
; 1333 :             if (_Old == _Myend) { // no more nodes to reuse, append the rest

	cmp	rdx, r8
	je	$LN948@RemoveNonT

; 1334 :                 _List_node_insert_op2<_Alnode> _Op(_Getal());
; 1335 :                 _Op._Append_range_unchecked(_STD move(_First), _Last);
; 1336 :                 _Op._Attach_at_end(_Mypair._Myval2);
; 1337 :                 return;
; 1338 :             }
; 1339 : 
; 1340 :             // reuse the node
; 1341 :             _Old->_Myval = *_First;

	mov	rax, QWORD PTR [rbx+16]
	mov	QWORD PTR [rdx+16], rax

; 1342 :             _Old         = _Old->_Next;

	mov	rdx, QWORD PTR [rdx]

; 50   :         _Ptr = _Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]

; 72   :         return _Ptr == _Right._Ptr;

	cmp	rbx, r12

; 1327 :             if (_First == _Last) {

	jne	SHORT $LL318@RemoveNonT
$LN947@RemoveNonT:

; 1328 :                 // input sequence exhausted; destroy and deallocate any tail of unneeded nodes
; 1329 :                 _Unchecked_erase(_Old, _Myend);

	mov	rcx, r15
	call	?_Unchecked_erase@?$list@_KV?$allocator@_K@std@@@std@@AEAAPEAU?$_List_node@_KPEAX@2@PEAU32@QEAU32@@Z ; std::list<unsigned __int64,std::allocator<unsigned __int64> >::_Unchecked_erase
	npad	1
$LN423@RemoveNonT:

; 1518 :         _Node::_Free_non_head(_Al, _My_data._Myhead);

	mov	rcx, QWORD PTR unBreakableCC$[rbp-89]

; 324  :         _Head->_Prev->_Next = nullptr;

	mov	rax, QWORD PTR [rcx+8]
	mov	QWORD PTR [rax], r13

; 325  : 
; 326  :         auto _Pnode = _Head->_Next;

	mov	rcx, QWORD PTR [rcx]

; 327  :         for (_Nodeptr _Pnext; _Pnode; _Pnode = _Pnext) {

	test	rcx, rcx
	je	SHORT $LN967@RemoveNonT
	npad	8
$LL87@RemoveNonT:

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
	jne	SHORT $LL87@RemoveNonT
$LN967@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	mov	rcx, QWORD PTR unBreakableCC$[rbp-89]
	call	??3@YAXPEAX_K@Z				; operator delete
	npad	1
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1518 :         _Node::_Free_non_head(_Al, _My_data._Myhead);

	mov	rcx, QWORD PTR breakableCC$[rbp-89]

; 324  :         _Head->_Prev->_Next = nullptr;

	mov	rax, QWORD PTR [rcx+8]
	mov	QWORD PTR [rax], r13

; 325  : 
; 326  :         auto _Pnode = _Head->_Next;

	mov	rcx, QWORD PTR [rcx]

; 327  :         for (_Nodeptr _Pnext; _Pnode; _Pnode = _Pnext) {

	test	rcx, rcx
	je	SHORT $LN968@RemoveNonT
	npad	9
$LL30@RemoveNonT:

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
	jne	SHORT $LL30@RemoveNonT
$LN968@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	mov	rcx, QWORD PTR breakableCC$[rbp-89]
	call	??3@YAXPEAX_K@Z				; operator delete
; File C:\Users\user\Documents\Codex\2026-09-22\referenced-chatgpt-conversation-this-is-an\work\v20\targets-build\generated\v19.inc

; 89   : }

	lea	r11, QWORD PTR [rsp+144]
	mov	rbx, QWORD PTR [r11+48]
	mov	rsi, QWORD PTR [r11+64]
	mov	rdi, QWORD PTR [r11+72]
	movaps	xmm6, XMMWORD PTR [r11-16]
	mov	rsp, r11
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	rbp
	ret	0
$LN948@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 627  :     explicit _List_node_insert_op2(_Alnode& _Al_) : _Al(_Al_), _Added(0) {}

	mov	QWORD PTR _Op$6[rbp-89], r15
	mov	r14, r13
	mov	QWORD PTR _Op$6[rbp-81], r13
	xorps	xmm0, xmm0

; 750  :             value_type::_Freenode(_Al, _STD exchange(_Subject, _Subject->_Next));
; 751  :         }
; 752  :     }
; 753  : 
; 754  : private:
; 755  :     _Alnode& _Al;
; 756  :     size_type _Added; // if 0, the values of _Head and _Tail are indeterminate
; 757  :     pointer _Tail{}; // points to the most recently appended element; it doesn't have _Next constructed

	movdqu	XMMWORD PTR _Op$6[rbp-73], xmm0

; 664  :         if (_First == _Last) { // throws

	cmp	rbx, r12
	je	SHORT $LN1007@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 1160 :     _CONSTEXPR20 explicit _Alloc_construct_ptr(_Alloc& _Al_) : _Al(_Al_), _Ptr(nullptr) {}

	mov	QWORD PTR _Newnode$3[rbp-89], r15

; 1167 :         _Ptr = nullptr; // if allocate throws, prevents double-free

	mov	QWORD PTR _Newnode$3[rbp-81], r13

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
	mov	rdi, rax
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 671  :             _Alnode_traits::construct(_Al, _STD addressof(_Newnode._Ptr->_Myval), *_First); // throws

	mov	rcx, QWORD PTR [rbx+16]
	mov	QWORD PTR [rax+16], rcx
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\utility

; 774  :     _Val         = static_cast<_Other&&>(_New_val);

	mov	QWORD PTR _Newnode$3[rbp-81], r13
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 673  :             _Head               = _Newhead;

	mov	QWORD PTR _Op$6[rbp-65], rax

; 674  :             _Tail               = _Newhead;

	mov	rsi, rax
	mov	QWORD PTR _Op$6[rbp-73], rax

; 675  :             ++_Added;

	mov	r14d, 1
	mov	QWORD PTR _Op$6[rbp-81], r14

; 50   :         _Ptr = _Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]

; 77   :         return !(*this == _Right);

	cmp	rbx, r12

; 679  :         while (_First != _Last) { // throws

	je	SHORT $LN330@RemoveNonT
	npad	1
$LL329@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 1167 :         _Ptr = nullptr; // if allocate throws, prevents double-free

	mov	QWORD PTR _Newnode$3[rbp-81], r13

; 136  :         return ::operator new(_Bytes);

	mov	ecx, 24
	call	??2@YAPEAX_K@Z				; operator new
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 681  :             _Alnode_traits::construct(_Al, _STD addressof(_Newnode._Ptr->_Myval), *_First); // throws

	mov	rcx, QWORD PTR [rbx+16]
	mov	QWORD PTR [rax+16], rcx

; 682  :             _Construct_in_place(_Tail->_Next, _Newnode._Ptr);

	mov	QWORD PTR [rsi], rax

; 683  :             _Construct_in_place(_Newnode._Ptr->_Prev, _Tail);

	mov	QWORD PTR [rax+8], rsi
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\utility

; 774  :     _Val         = static_cast<_Other&&>(_New_val);

	mov	QWORD PTR _Newnode$3[rbp-81], r13
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 684  :             _Tail = _STD exchange(_Newnode._Ptr, pointer{});

	mov	rsi, rax
	mov	QWORD PTR _Op$6[rbp-73], rax

; 685  :             ++_Added;

	inc	r14
	mov	QWORD PTR _Op$6[rbp-81], r14

; 50   :         _Ptr = _Ptr->_Next;

	mov	rbx, QWORD PTR [rbx]

; 77   :         return !(*this == _Right);

	cmp	rbx, r12

; 679  :         while (_First != _Last) { // throws

	jne	SHORT $LL329@RemoveNonT
$LN330@RemoveNonT:

; 628  : 
; 629  :     _List_node_insert_op2(const _List_node_insert_op2&)            = delete;
; 630  :     _List_node_insert_op2& operator=(const _List_node_insert_op2&) = delete;
; 631  : 
; 632  :     template <class... _CArgT>
; 633  :     void _Append_n(size_type _Count, const _CArgT&... _Carg) {
; 634  :         // Append _Count elements constructed from _Carg
; 635  :         if (_Count <= 0) {
; 636  :             return;
; 637  :         }
; 638  : 
; 639  :         _Alloc_construct_ptr<_Alnode> _Newnode(_Al);
; 640  :         if (_Added == 0) {
; 641  :             _Newnode._Allocate(); // throws
; 642  :             _Alnode_traits::construct(_Al, _STD addressof(_Newnode._Ptr->_Myval), _Carg...); // throws
; 643  :             _Head = _Newnode._Ptr;
; 644  :             _Tail = _Newnode._Ptr;
; 645  :             ++_Added;
; 646  :             --_Count;
; 647  :         }
; 648  : 
; 649  :         for (; 0 < _Count; --_Count) {
; 650  :             _Newnode._Allocate(); // throws
; 651  :             _Alnode_traits::construct(_Al, _STD addressof(_Newnode._Ptr->_Myval), _Carg...); // throws
; 652  :             _Construct_in_place(_Tail->_Next, _Newnode._Ptr);
; 653  :             _Construct_in_place(_Newnode._Ptr->_Prev, _Tail);
; 654  :             _Tail = _Newnode._Ptr;
; 655  :             ++_Added;
; 656  :         }
; 657  : 
; 658  :         _Newnode._Ptr = pointer{};
; 659  :     }
; 660  : 
; 661  :     template <class _InIt, class _Sentinel>
; 662  :     void _Append_range_unchecked(_InIt _First, const _Sentinel _Last) {
; 663  :         // Append the values in [_First, _Last)
; 664  :         if (_First == _Last) { // throws

	jmp	SHORT $LN380@RemoveNonT
$LN1007@RemoveNonT:
	mov	rdi, QWORD PTR _Op$6[rbp-65]
	mov	rsi, QWORD PTR _Op$6[rbp-73]
$LN380@RemoveNonT:

; 717  :         _Attach_before(_List_data, _List_data._Myhead);

	mov	rcx, QWORD PTR [r15]

; 697  :         if (_Local_added == 0) {

	test	r14, r14
	je	SHORT $LN417@RemoveNonT

; 698  :             return _Insert_before;
; 699  :         }
; 700  : 
; 701  :         const auto _Local_head   = _Head;
; 702  :         const auto _Local_tail   = _Tail;
; 703  :         const auto _Insert_after = _Insert_before->_Prev;

	mov	rax, QWORD PTR [rcx+8]

; 704  : 
; 705  :         _Construct_in_place(_Local_head->_Prev, _Insert_after);

	mov	QWORD PTR [rdi+8], rax

; 706  :         _Insert_after->_Next = _Local_head;

	mov	QWORD PTR [rax], rdi

; 707  :         _Construct_in_place(_Local_tail->_Next, _Insert_before);

	mov	QWORD PTR [rsi], rcx

; 708  :         _Insert_before->_Prev = _Local_tail;

	mov	QWORD PTR [rcx+8], rsi

; 709  : 
; 710  :         _List_data._Mysize += _Local_added;

	add	QWORD PTR [r15+8], r14

; 711  :         _Added = 0;

	mov	r14, r13
$LN417@RemoveNonT:

; 742  :         if (_Added == 0) {

	test	r14, r14
	je	$LN423@RemoveNonT

; 743  :             return;
; 744  :         }
; 745  : 
; 746  :         _Construct_in_place(_Head->_Prev, pointer{});

	mov	QWORD PTR [rdi+8], r13

; 747  :         _Construct_in_place(_Tail->_Next, pointer{});

	mov	QWORD PTR [rsi], r13
	npad	14
$LL422@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\utility

; 773  :     _Ty _Old_val = static_cast<_Ty&&>(_Val);

	mov	rcx, rdi

; 774  :     _Val         = static_cast<_Other&&>(_New_val);

	mov	rdi, QWORD PTR [rdi]
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 289  :         ::operator delete(_Ptr, _Bytes);

	mov	edx, 24
	call	??3@YAXPEAX_K@Z				; operator delete
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 749  :         while (_Subject) {

	test	rdi, rdi
	jne	SHORT $LL422@RemoveNonT
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\xmemory

; 219  :     _STL_VERIFY(_Back_shift >= _Min_back_shift && _Back_shift <= _Non_user_size, "invalid argument");

	jmp	$LN423@RemoveNonT
$LN939@RemoveNonT:
; File C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\include\list

; 1031 :             _Xlength_error("list too long");

	lea	rcx, OFFSET FLAT:??_C@_0O@NKNMEGII@list?5too?5long@
	call	?_Xlength_error@std@@YAXPEBD@Z		; std::_Xlength_error
	int	3
$LN1012@RemoveNonT:
?RemoveNonThreating@PossibleAttackTargetsValue@v19@@QEAAXAEAV?$list@_KV?$allocator@_K@std@@@std@@_N@Z ENDP ; v19::PossibleAttackTargetsValue::RemoveNonThreating