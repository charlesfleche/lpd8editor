create trigger aft_insert after insert on programs
begin

insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 1, 36, 1, 48, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 2, 37, 2, 49, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 3, 38, 3, 50, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 4, 39, 4, 51, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 5, 40, 5, 16, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 6, 41, 6, 17, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 7, 42, 7, 18, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 8, 43, 8, 19, 0);

insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 1, 16, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 2, 17, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 3, 18, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 4, 19, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 5, 48, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 6, 49, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 7, 50, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 8, 51, 0, 127);

end;
