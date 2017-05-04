create trigger aft_insert after insert on programs
begin

insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 1, 36, 1, 1, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 2, 37, 2, 2, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 3, 38, 3, 3, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 4, 39, 4, 4, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 5, 40, 5, 5, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 6, 41, 6, 6, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 7, 42, 7, 7, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 8, 43, 8, 8, 0);

insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 1,  9, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 2, 10, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 3, 11, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 4, 12, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 5, 13, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 6, 14, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 7, 15, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 8, 16, 0, 127);

end;
