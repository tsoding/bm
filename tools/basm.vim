" Vim syntax file
" Language: Basm

" Usage Instructions
" Put this file in .vim/syntax/basm.vim
" and add in your .vimrc file the next line:
" autocmd BufRead,BufNewFile *.basm set filetype=basm

if exists("b:current_syntax")
  finish
endif

syntax keyword basmTodos TODO XXX FIXME NOTE

" Language keywords
syntax keyword basmKeywords nop push drop dup
syntax keyword basmKeywords plusi minusi multi divi modi
syntax keyword basmKeywords multu divu modu
syntax keyword basmKeywords plusf minusf multf divf
syntax keyword basmKeywords jmp jmp_if halt swap not
syntax keyword basmKeywords eqi gei gti lei lti nei
syntax keyword basmKeywords equ geu gtu leu ltu neu
syntax keyword basmKeywords eqf gef gtf lef ltf nef
syntax keyword basmKeywords ret call native
syntax keyword basmKeywords andb orb xor shr shl notb
syntax keyword basmKeywords read8 read16 read32 read64
syntax keyword basmKeywords write8 write16 write32 write64
syntax keyword basmKeywords i2f u2f f2i f2u

" Comments
syntax region basmCommentLine start=";" end="$"   contains=basmTodos
syntax region basmDirective start="%" end=" "

" Numbers
syntax match basmDecInt display "\<[0-9][0-9_]*"
syntax match basmHexInt display "\<0[xX][0-9a-fA-F][0-9_a-fA-F]*"
syntax match basmFloat  display "\<[0-9][0-9_]*\%(\.[0-9][0-9_]*\)"

" Strings
syntax region basmString start=/\v"/ skip=/\v\\./ end=/\v"/
syntax region basmString start=/\v'/ skip=/\v\\./ end=/\v'/

" Set highlights
highlight default link basmTodos Todo
highlight default link basmKeywords Keyword
highlight default link basmCommentLine Comment
highlight default link basmDirective PreProc
highlight default link basmDecInt Number
highlight default link basmHexInt Number
highlight default link basmFloat Float
highlight default link basmString String

let b:current_syntax = "basm"
