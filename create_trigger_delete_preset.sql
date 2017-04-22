create trigger aft_delete after delete on presets
begin
delete from pads where presetId = OLD.presetId;
delete from knobs where presetId = OLD.presetId;
end;
