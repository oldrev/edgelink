use thiserror::Error;

#[derive(Error, Debug)]
pub enum PropexError {
    #[error("Invalid arguments")]
    BadArguments,

    #[error("Invalid Propex syntax")]
    BadSyntax(String),

    #[error("Invalid number digit")]
    InvalidDigit,
}

#[derive(Copy, Clone)]
pub enum PropexSegment<'a> {
    IntegerIndex(usize),
    StringIndex(&'a str), // Use a reference to a string slice
}

struct PropexEval<'a> {
    expr: &'a str,
    pos: std::cell::Cell<usize>,
}

pub fn parse(expr: &str) -> Result<Vec<PropexSegment<'_>>, PropexError> {
    let eval = PropexEval {
        expr,
        pos: std::cell::Cell::new(0),
    };
    let mut segs = Vec::new();
    let mut _levels: usize = 0;

    eval.skip_whitespace();

    while eval.is_available() {
        /*
        if levels >= PROPEX_MAX_LEVELS {
            return Err(PropexError::BadArguments);
        }
        */
        eval.skip_whitespace();
        if eval.peek() == '.' {
            eval.forward(1);
            continue;
        } else if eval.peek() == '[' {
            eval.forward(1);
            eval.skip_whitespace();
            if eval.peek() == '\'' || eval.peek() == '"' {
                let seg = eval.parse_string_index()?;
                segs.push(seg);
            } else {
                let seg = eval.parse_integer_index()?;
                segs.push(seg);
            }
            eval.skip_whitespace();
            if eval.peek() != ']' {
                return Err(PropexError::BadSyntax("Unmatched ']'".to_string()));
            }
            eval.forward(1);
        } else if eval.peek().is_ascii_alphabetic() || eval.peek() == '_' {
            let seg = eval.parse_property()?;
            segs.push(seg);
            assert_eq!(eval.peek(), '.');
        } else {
            return Err(PropexError::BadSyntax(
                "Bad property expression".to_string(),
            ));
        }

        _levels += 1;
        eval.skip_whitespace();
    }

    Ok(segs)
}

impl<'a> PropexEval<'a> {
    fn is_available(&self) -> bool {
        self.pos.get() < self.expr.len()
    }

    fn forward(&self, n: usize) -> char {
        let new_pos = self.pos.get() + n;
        assert!(new_pos < self.expr.len());
        self.pos.set(new_pos);
        self.peek()
    }

    fn peek(&self) -> char {
        assert!(self.pos.get() < self.expr.len());
        self.expr.chars().nth(self.pos.get()).unwrap()
    }

    fn forward_when(&self, pred: fn(char) -> bool) {
        while pred(self.peek()) {
            self.forward(1);
        }
    }

    fn scan_for(&self, pred: fn(char) -> bool) {
        while self.is_available() && pred(self.peek()) {
            self.forward(1);
        }
    }

    fn skip_whitespace(&self) {
        self.forward_when(|c| c == '\t' || c == ' ')
    }

    fn parse_integer_index(&self) -> Result<PropexSegment<'a>, PropexError> {
        let start_pos = self.pos.get();
        self.forward_when(|c| c.is_ascii_digit());
        let digits = &self.expr[start_pos..self.pos.get()];
        match digits.parse::<usize>() {
            Ok(index) => Ok(PropexSegment::IntegerIndex(index)),
            _ => Err(PropexError::InvalidDigit),
        }
    }

    fn parse_property(&self) -> Result<PropexSegment<'a>, PropexError> {
        let start_pos = self.pos.get();
        self.scan_for(|c| c.is_ascii_alphanumeric() || c == '_');

        Ok(PropexSegment::StringIndex(
            &self.expr[start_pos..self.pos.get()],
        ))
    }

    fn parse_string_index(&self) -> Result<PropexSegment<'a>, PropexError> {
        self.forward(1); // skip '\'' or '"'
        let start_pos = self.pos.get();

        // scan for string content
        self.scan_for(|c| c == '\'' || c == '"');

        if self.peek() != '\'' && self.peek() != '"' {
            return Err(PropexError::BadSyntax(
                "Unmatched string quotation".to_string(),
            ));
        }
        _ = self.forward(1);
        Ok(PropexSegment::StringIndex(
            &self.expr[start_pos..self.pos.get()],
        ))
    }
}

#[test]
fn all_primitives_should_work() {
    let white_expr = "   aaabcd";
    let eval = PropexEval {
        expr: white_expr,
        pos: std::cell::Cell::new(0),
    };
    eval.skip_whitespace();
    assert_eq!(3, eval.pos.get());
    eval.forward(1);
    assert_eq!('a', eval.peek());
    eval.scan_for(|c| c == 'a');
    assert_eq!('b', eval.peek());
    eval.forward(2);
    assert_eq!('d', eval.peek());
}

#[test]
fn parse_propex_should_be_ok() {
    //let expr1 = "test1.hello .world[ 'aaa' ].name_of";
    let expr1 = "test1.name_of";
    let segs = parse(expr1).unwrap();

    match segs[0] {
        PropexSegment::StringIndex(str) => assert_eq!(str, "hello"),
        _ => assert!(false),
    };
}
