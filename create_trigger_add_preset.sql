create trigger aft_insert after insert on presets
begin

insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 1, 1, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 1, 2, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 1, 3, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 1, 4, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 1, 5, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 1, 6, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 1, 7, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 1, 8, 1, 2, 3, 0);

insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 1, 1, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 1, 2, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 1, 3, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 1, 4, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 1, 5, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 1, 6, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 1, 7, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 1, 8, 1, 0, 127);

insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 2, 1, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 2, 2, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 2, 3, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 2, 4, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 2, 5, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 2, 6, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 2, 7, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 2, 8, 1, 2, 3, 0);

insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 2, 1, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 2, 2, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 2, 3, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 2, 4, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 2, 5, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 2, 6, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 2, 7, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 2, 8, 1, 0, 127);

insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 3, 1, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 3, 2, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 3, 3, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 3, 4, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 3, 5, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 3, 6, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 3, 7, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 3, 8, 1, 2, 3, 0);

insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 3, 1, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 3, 2, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 3, 3, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 3, 4, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 3, 5, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 3, 6, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 3, 7, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 3, 8, 1, 0, 127);

insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 4, 1, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 4, 2, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 4, 3, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 4, 4, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 4, 5, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 4, 6, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 4, 7, 1, 2, 3, 0);
insert into pads(presetId, programId, controlId, note, pc, cc, toggle) values(NEW.presetId, 4, 8, 1, 2, 3, 0);

insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 4, 1, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 4, 2, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 4, 3, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 4, 4, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 4, 5, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 4, 6, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 4, 7, 1, 0, 127);
insert into knobs(presetId, programId, controlId, cc, low, high) values(NEW.presetId, 4, 8, 1, 0, 127);

end;
