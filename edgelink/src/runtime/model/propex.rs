use thiserror::Error;

use nom::{
    branch::alt,
    bytes::complete::{escaped, tag, take_while},
    character::complete::{alphanumeric1 as alphanumeric, char, one_of},
    combinator::{cut, map, opt, value},
    error::{context, convert_error, ContextError, ErrorKind, ParseError, VerboseError},
    multi::separated_list0,
    number::complete::double,
    sequence::{delimited, preceded, separated_pair, terminated},
    Err, IResult, Parser,
};

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

#[derive(Debug)]
enum NavigationElement {
    Field(String),
    StringIndex(String),
}

fn skip_spaces<'a, E: ParseError<&'a str>>(i: &'a str) -> IResult<&'a str, &'a str, E> {
    let chars = " \t\r\n";
    take_while(move |c| chars.contains(c))(i)
}

fn parse_str<'a, E: ParseError<&'a str>>(i: &'a str) -> IResult<&'a str, &'a str, E> {
    escaped(alphanumeric, '\\', one_of("\"n\\"))(i)
}

fn parse_double_quoted_string<'a, E: ParseError<&'a str> + ContextError<&'a str>>(
    i: &'a str,
) -> IResult<&'a str, &'a str, E> {
    context(
        "double_quoted_string",
        preceded(char('\"'), cut(terminated(parse_str, char('\"')))),
    )(i)
}

fn parse_single_quoted_string<'a, E: ParseError<&'a str> + ContextError<&'a str>>(
    i: &'a str,
) -> IResult<&'a str, &'a str, E> {
    context(
        "double_quoted_string",
        preceded(char('\''), cut(terminated(parse_str, char('\'')))),
    )(i)
}

pub fn parse(expr: &str) -> Result<Vec<PropexSegment<'_>>, PropexError> {
    let mut segs = Vec::new();
    let mut _levels: usize = 0;

    /*
    let mut lex = Token::lexer(expr);

    while let Some(token) = lex.next() {
        let seg = match token {
            Ok(Token::Dot) => continue,
            Ok(Token::DoubleQuotedString) => PropexSegment::StringIndex(lex.slice()),
            Err(err) => return Err(PropexError::BadSyntax("Syntax error".to_string())),
            _ => PropexSegment::IntegerIndex(123),
        };
        segs.push(seg);
    }
    */

    Ok(segs)
}

#[test]
fn parse_propex_should_be_ok() {
    let expr1 = "test1.hello .world['aaa'].name_of";
    let segs = parse(expr1).unwrap();

    match segs[0] {
        PropexSegment::StringIndex(str) => assert_eq!(str, "hello"),
        _ => assert!(false),
    };
}
