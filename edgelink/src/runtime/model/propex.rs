use thiserror::Error;

use nom::{
    branch::alt,
    bytes::complete::{escaped, tag, take_while},
    character::complete::{alpha1, alphanumeric1, char, digit1, multispace0, one_of},
    combinator::{cut, iterator, map, map_res, opt, recognize, value},
    error::{context, convert_error, ContextError, ErrorKind, ParseError, VerboseError},
    multi::{many0, many0_count, separated_list0},
    number::complete::double,
    sequence::{self, delimited, pair, preceded, separated_pair, terminated},
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

#[derive(Debug, Copy, Clone, PartialEq)]
pub enum PropexSegment<'a> {
    IntegerIndex(usize),
    StringIndex(&'a str), // Use a reference to a string slice
}

fn parse_identifier(i: &str) -> IResult<&str, &str, VerboseError<&str>> {
    recognize(pair(
        alt((alpha1, tag("_"))),
        many0_count(alt((alphanumeric1, tag("_")))),
    ))
    .parse(i)
}

fn parse_double_quoted_string(i: &str) -> IResult<&str, PropexSegment, VerboseError<&str>> {
    map(
        context(
            "double_quoted_string",
            preceded(char('\"'), cut(terminated(parse_str, char('\"')))),
        ),
        PropexSegment::StringIndex,
    )
    .parse(i)
}

fn parse_single_quoted_string(i: &str) -> IResult<&str, PropexSegment, VerboseError<&str>> {
    map(
        context(
            "double_quoted_string",
            preceded(char('\''), cut(terminated(parse_str, char('\'')))),
        ),
        PropexSegment::StringIndex,
    )
    .parse(i)
}

fn parse_quoted_string(i: &str) -> IResult<&str, PropexSegment, VerboseError<&str>> {
    alt((parse_double_quoted_string, parse_single_quoted_string)).parse(i)
}

fn parse_str<'a, E: ParseError<&'a str>>(i: &'a str) -> IResult<&'a str, &'a str, E> {
    escaped(alphanumeric1, '\\', one_of("\"n\\"))(i)
}

fn parse_integer_index(i: &str) -> IResult<&str, PropexSegment, VerboseError<&str>> {
    map_res(digit1, |digit_str: &str| {
        digit_str.parse::<usize>().map(PropexSegment::IntegerIndex)
    })
    .parse(i)
}

fn parse_index(i: &str) -> IResult<&str, PropexSegment, VerboseError<&str>> {
    delimited(
        delimited(multispace0, char('['), multispace0),
        alt((
            parse_double_quoted_string,
            parse_single_quoted_string,
            parse_integer_index,
        )),
        delimited(multispace0, char(']'), multispace0),
    )
    .parse(i)
}

fn parse_property(i: &str) -> IResult<&str, PropexSegment, VerboseError<&str>> {
    map(
        context(
            "property",
            delimited(multispace0, parse_identifier, multispace0),
        ),
        PropexSegment::StringIndex,
    )
    .parse(i)
}

fn parse_subproperty(i: &str) -> IResult<&str, PropexSegment, VerboseError<&str>> {
    map(
        context(
            "subproperty",
            preceded(
                delimited(multispace0, char('.'), multispace0),
                delimited(multispace0, parse_identifier, multispace0),
            ),
        ),
        PropexSegment::StringIndex,
    )
    .parse(i)
}

fn parse_first_fragment(i: &str) -> IResult<&str, PropexSegment, VerboseError<&str>> {
    alt((parse_property, parse_index)).parse(i)
}

fn parse_sub_fragment(i: &str) -> IResult<&str, PropexSegment, VerboseError<&str>> {
    alt((parse_subproperty, parse_index)).parse(i)
}

fn parse_nav(i: &str) -> IResult<&str, Vec<PropexSegment>, VerboseError<&str>> {
    let mut segs = Vec::new();
    let p = pair(parse_first_fragment, many0(parse_sub_fragment)).parse(i)?;
    segs.push(p.1 .0.clone());
    for x in p.1 .1.iter() {
        segs.push(*x);
    }
    // parse_first_fragment.map(move |x| segs.push(x)).parse(i)?;
    Ok((i, segs))
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
fn parse_primitives_should_be_ok() {
    let expr = "'test1'";
    let (_, parsed) = parse_single_quoted_string(expr).unwrap();
    assert_eq!(PropexSegment::StringIndex("test1"), parsed);

    let expr = "\"test1\"";
    let (_, parsed) = parse_double_quoted_string(expr).unwrap();
    assert_eq!(PropexSegment::StringIndex("test1"), parsed);

    let expr = "_test_1";
    let (_, parsed) = parse_property(expr).unwrap();
    assert_eq!(PropexSegment::StringIndex("_test_1"), parsed);

    let expr = "['aaa']";
    let (_, parsed) = parse_index(expr).unwrap();
    assert_eq!(PropexSegment::StringIndex("aaa"), parsed);

    let expr = "[ 123 ]";
    let (_, parsed) = parse_index(expr).unwrap();
    assert_eq!(PropexSegment::IntegerIndex(123), parsed);
}

#[test]
fn parse_propex_should_be_ok() {
    let expr1 = "test1.hello .world['aaa'][333][\"bb\"].name_of";
    let segs = parse_nav(expr1).unwrap().1;

    assert_eq!(7, segs.len());
    assert_eq!(PropexSegment::StringIndex("test1"), segs[0]);
    assert_eq!(PropexSegment::StringIndex("hello"), segs[1]);
    assert_eq!(PropexSegment::StringIndex("world"), segs[2]);
    assert_eq!(PropexSegment::StringIndex("aaa"), segs[3]);
    assert_eq!(PropexSegment::IntegerIndex(333), segs[4]);
    assert_eq!(PropexSegment::StringIndex("bb"), segs[5]);
    assert_eq!(PropexSegment::StringIndex("name_of"), segs[6]);
}

