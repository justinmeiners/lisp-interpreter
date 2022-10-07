; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_7.html

; TODO: add characters to reader
(==> (make-string 10 #\x) "xxxxxxxxxx")

(assert (string? "Hi"))
(assert (not (string? 'Hi)))

(==> (string-length "") 0)
(==> (string-length "The length") 10)

(assert (string=? "PIE" "PIE"))
(assert (not (string=? "PIE" "pie")))

(==> (list->string (string->list "hello 123")) "hello 123")
(==> (string->list (list->string '(#\A #\B #\3))) (#\A #\B #\3))


; https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Symbols.html
(assert (symbol? 'foo))
(assert (symbol? (car '(a b))))
(assert (not (symbol? "bar")))

(assert (eq? 'foo (string->symbol "FOO")))
(assert (string=? "FLYING-FISH" (symbol->string 'flying-fish)))

; specials
(==> (string-length "\\") 1)
(==> (string-length "\t") 1)
(==> (string-length "\n") 1)
(==> (string-length "\f") 1)
(==> (string-length "\"") 1)

(display "Hello\nworld!")

(==> (string->number (number->string 279)) 279)
(==> (number->string (string->number "279")) "279")
(==> (string->number (number->string 0.5)) 0.5)

(assert (symbol<? 'A 'B))
(assert (not (symbol<? 'WALK 'DOG)))

(==> (- (char->integer #\c) (char->integer #\a)) 2)

(==> (string-ref "abc" 0) #\a)
(==> (string-ref "abc" 2) #\c)
(==> (string #\a #\b) "ab")
(==> (string) "")

(assert (char<? #\a #\b))
(assert (char<=? #\a #\a))

(assert (char-lower-case? #\a))
(assert (not (char-lower-case? #\A)))

(assert (not (char-upper-case? #\c)))
(assert (char-upper-case? #\C))

(assert (char-ci=? #\a #\A))
(assert (char-ci<? #\A #\b))
