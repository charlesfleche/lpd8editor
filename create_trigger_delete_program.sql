create trigger aft_delete after delete on programs
begin
delete from pads where programId = OLD.programId;
delete from knobs where programId = OLD.programId;
end;
