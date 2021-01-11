;;; References:
;;; - http://ergoemacs.org/emacs/elisp_syntax_coloring.html

(defvar basm-mode-syntax-table nil "Syntax table for `basm-mode'.")

(setq basm-mode-syntax-table
      (let ((syn-table (make-syntax-table)))
        ;; assmebly style comment: “; …”
        (modify-syntax-entry ?\; "<" syn-table)
        (modify-syntax-entry ?\n ">" syn-table)
        syn-table))

(setq basm-highlights
      '(("%[[:word:]_]+" . font-lock-preprocessor-face)
        ("[[:word:]_]+\\:" . font-lock-constant-face)
        ("nop\\|push\\|drop\\|dup\\|plusi\\|minusi\\|multi\\|divi\\|modi\\|plusf\\|minusf\\|multf\\|divf\\|jmp_if\\|jmp\\|eq\\|halt\\|swap\\|not\\|gef\\|gei\\|ret\\|call\\|native\\|andb\\|orb\\|xor\\|shr\\|shl\\|notb\\|read8\\|read16\\|read32\\|read64\\|write8\\|write16\\|write32\\|write64" . font-lock-keyword-face)))

(define-derived-mode basm-mode fundamental-mode "basm"
  "Major Mode for editing BASM Assembly Code."
  (setq font-lock-defaults '(basm-highlights))
  (set-syntax-table basm-mode-syntax-table))
