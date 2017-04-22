create trigger aft_insert after insert on programs
begin

insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 1, 1, 2, 3, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 2, 1, 2, 3, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 3, 1, 2, 3, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 4, 1, 2, 3, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 5, 1, 2, 3, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 6, 1, 2, 3, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 7, 1, 2, 3, 0);
insert into pads(programId, controlId, note, pc, cc, toggle) values(NEW.programId, 8, 1, 2, 3, 0);

insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 1, 1, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 2, 1, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 3, 1, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 4, 1, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 5, 1, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 6, 1, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 7, 1, 0, 127);
insert into knobs(programId, controlId, cc, low, high) values(NEW.programId, 8, 1, 0, 127);

end;
