%%% @author Tony Rogvall <tony@rogvall.se>
%%% @copyright (C) 2024, Tony Rogvall
%%% @doc
%%%     Font file to besk data
%%%     each glyph is 5x7 bits (35 bits)
%%% @end
%%% Created :  3 Jan 2024 by Tony Rogvall <tony@rogvall.se>

-module(font_to_besk_data).
-export([file/1, file/3]).

file(FileName) ->
    file(FileName, 16#200, []).

file(FileName,StartAddr,Filter) ->
    case file:open(FileName,[read,binary]) of
	{ok,Fd} ->
	    try read_font(Fd) of
		{ok,Font} ->
		    file:close(Fd),
		    Gs = font_to_bits(lists:reverse(Font)),
		    Gs1 = glyphs_to_40bit(Gs),
		    format_glyph_data(StartAddr, Gs1, Filter),
		    {ok,Gs1};
		{error,Reason} ->
		    {error,Reason}
	    after
		file:close(Fd)
	    end;
	{error,Reason} ->
	    {error, Reason}
    end.
%%
%% Font data comes in 5x7 bits (35 bits)
%% like |abcde|fghij|klmno|pqrst|uvxyz|ABCDE|FGHIJ|
%%  row |  0  |  1  |  2  |  3  |  4  |  5  |  6  |
%% and is stored in a 40 bit left shifted (5 bits)
%%   |abcde|fghij|klmno|pqrst|uvxyz|ABCDE|FGHIJ|00000|
%% 

format_glyph_data(Addr,[{Code,Data}|Gs], Filter) ->
    case (Filter =:= []) orelse lists:member(Code, Filter) of
	true ->
	    Data40 = Data bsl 5,  %% left shift 5 bits, adding a blank row
	    io:format("# letter ~w [~ts]\n",[Code,[Code]]),
	    io:format("~3.16.0B ~5.16.0B\n", [Addr, (Data40 bsr 20) band 16#fffff]),
	    io:format("~3.16.0B ~5.16.0B\n", [Addr+1, Data40 band 16#fffff]),
	    format_glyph_data(Addr+2,Gs,Filter);
	false ->
	    format_glyph_data(Addr,Gs,Filter)
    end;
format_glyph_data(Addr, [], _Filter) ->
    {ok, Addr}.

glyphs_to_40bit([{Code, Bits}|Gs]) ->
    Glyph = lists:foldl(fun(B,Acc) -> (Acc bsl 5) bor B end, 0, Bits),
    [{Code, Glyph}|glyphs_to_40bit(Gs)];
glyphs_to_40bit([]) ->
    [].

font_to_bits(Lines) ->
    font_to_bits(Lines, []).
font_to_bits([Info|Lines], Gs) ->
    %% io:format("Info: ~p~n",[Info]),
    case binary:split(Info, <<" ">>, [global,trim_all]) of
	[Num, <<Code/utf8>>] ->
	    _ = binary_to_integer(Num),
	    font_to_bits(Lines, Code, [], Gs);
	[<<Code/utf8>>] ->
	    font_to_bits(Lines, Code, [], Gs)
    end;
font_to_bits([], Gs) ->
    lists:reverse(Gs).

font_to_bits([<<>>|Lines], Code, Bits, Gs) ->
    font_to_bits(Lines, [{Code, lists:reverse(Bits)}|Gs]);
font_to_bits([Line|Lines], Code, Bits, Gs) ->
    L = line_to_bits(Line, 0),
    font_to_bits(Lines, Code, [L|Bits], Gs);
font_to_bits([], Code, Bits, Gs) ->
    lists:reverse([{Code, lists:reverse(Bits)}|Gs]).

line_to_bits(<<$.,Line/binary>>, N) ->
    line_to_bits(Line, (N bsl 1));
line_to_bits(<<$*,Line/binary>>, N) ->
    line_to_bits(Line, (N bsl 1) bor 1);
line_to_bits(<<>>, N) ->
    N.

read_font(Fd) ->
    read_font(Fd, []).

read_font(Fd, Lines) ->
    case file:read_line(Fd) of
	{error,Reason} ->
	    {error,Reason};
	eof ->
	    {ok, Lines};
	{ok,Line0} ->
	    Length = byte_size(Line0),
	    case Line0 of
		<<Line:(Length-2)/binary,"\r\n">> ->
		    read_font(Fd, [Line|Lines]);
		<<Line:(Length-1)/binary,"\n">> ->
		    read_font(Fd, [Line|Lines]);
		<<Line:(Length-1)/binary,"\r">> ->
		    read_font(Fd, [Line|Lines])
	    end
    end.
		
