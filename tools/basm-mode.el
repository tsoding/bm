;;; References:
;;; - http://ergoemacs.org/emacs/elisp_syntax_coloring.html

(setq basm-highlights
      '(("nop\\|push\\|drop\\|dup\\|plusi\\|minusi\\|multi\\|divi\\|modi\\|plusf\\|minusf\\|multf\\|divf\\|jmp_if\\|jmp\\|eq\\|halt\\|swap\\|not\\|gef\\|gei\\|ret\\|call\\|native\\|andb\\|orb\\|xor\\|shr\\|shl\\|notb\\|read8\\|read16\\|read32\\|read64\\|write8\\|write16\\|write32\\|write64" . font-lock-keyword-face)))

(define-derived-mode basm-mode fundamental-mode "basm"
  "Major Mode for editing BASM Assembly Code."
  (setq font-lock-defaults '(basm-highlights)))
