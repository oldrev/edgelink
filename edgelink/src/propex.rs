use thiserror::Error;

#[derive(Error, Debug)]
pub enum PropexError {
    #[error("Invalid arguments")]
    BadArguments,

    #[error("Invalid Propex syntax")]
    BadSyntax,

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
        expr: expr,
        pos: std::cell::Cell::new(0),
    };
    let mut segs = Vec::new();
    let mut _levels: usize = 0;

    eval.skip_whitespace()?;

    while eval.pos.get() < eval.expr.len() {
        /*
        if levels >= PROPEX_MAX_LEVELS {
            return Err(PropexError::BadArguments);
        }
        */
        eval.skip_whitespace()?;
        if eval.peek()? == '.' {
            eval.forward(1)?;
            continue;
        } else if eval.peek()? == '[' {
            eval.forward(1)?;
            eval.skip_whitespace()?;
            if eval.peek()? == '\'' || eval.peek()? == '"' {
                let seg = eval.parse_string_index()?;
                segs.push(seg);
            } else {
                let seg = eval.parse_integer_index()?;
                segs.push(seg);
            }
            eval.skip_whitespace()?;
            if eval.peek()? != ']' {
                return Err(PropexError::BadSyntax);
            }
            eval.forward(1)?;
        } else if eval.peek()?.is_ascii_alphabetic() || eval.peek()? == '_' {
            let seg = eval.parse_property()?;
            segs.push(seg);
        } else {
            return Err(PropexError::BadSyntax);
        }

        _levels += 1;
        eval.skip_whitespace()?;
    }

    Ok(segs)
}

impl<'a> PropexEval<'a> {
    fn forward(&self, n: usize) -> Result<char, PropexError> {
        self.pos.set(self.pos.get() + n);
        self.peek()
    }

    fn peek(&self) -> Result<char, PropexError> {
        if self.pos.get() >= self.expr.len() {
            Err(PropexError::BadSyntax)
        } else if let Some(c) = self.expr.chars().nth(self.pos.get()) {
            Ok(c)
        } else {
            Err(PropexError::BadSyntax)
        }
    }

    fn forward_when(&self, pred: fn(char) -> bool) -> Result<(), PropexError> {
        while pred(self.peek()?) {
            self.forward(1)?;
        }
        Ok(())
    }

    fn scan_for(&self, pred: fn(char) -> bool) -> Result<(), PropexError> {
        while pred(self.peek()?) {
            self.forward(1)?;
        }
        Ok(())
    }

    fn skip_whitespace(&self) -> Result<(), PropexError> {
        self.forward_when(|c| c == '\t' || c == ' ')
    }

    fn parse_integer_index(&self) -> Result<PropexSegment<'a>, PropexError> {
        let start_pos = self.pos.get();
        self.forward_when(|c| c.is_ascii_digit())?;
        let digits = &self.expr[start_pos..self.pos.get()];
        match digits.parse::<usize>() {
            Ok(index) => Ok(PropexSegment::IntegerIndex(index)),
            _ => Err(PropexError::InvalidDigit),
        }
    }

    fn parse_property(&self) -> Result<PropexSegment<'a>, PropexError> {
        self.forward(1)?; // skip '\'' or '"'
        let start_pos = self.pos.get();
        if self.peek()?.is_ascii_alphabetic() || self.peek()? == '_' {
            self.scan_for(|c| c.is_ascii_alphanumeric() || c == '_')?;
        }

        Ok(PropexSegment::StringIndex(
            &self.expr[start_pos..self.pos.get()],
        ))
    }

    fn parse_string_index(&self) -> Result<PropexSegment<'a>, PropexError> {
        self.forward(1)?; // skip '\'' or '"'
        let start_pos = self.pos.get();

        // scan for string content
        self.scan_for(|c| c == '\'' || c == '"')?;

        if self.peek()? != '\'' && self.peek()? != '"' {
            return Err(PropexError::BadSyntax);
        }
        _ = self.forward(1)?;
        Ok(PropexSegment::StringIndex(
            &self.expr[start_pos..self.pos.get()],
        ))
    }
}

#[test]
fn parse_propex_should_be_ok() {
    let expr1 = "test1.hello .world[ 'aaa' ].name_of";
    let segs = parse(expr1).unwrap();

    match segs[0] {
        PropexSegment::StringIndex(str) => assert_eq!(str, "hello"),
        _ => assert!(false),
    };
}
