create table knobs(presetId integer,
                   programId integer NOT NULL,
                   controlId integer NOT NULL,
                   cc integer NOT NULL,
                   low integer NOT NULL,
                   high integer NOT NULL,
                   PRIMARY KEY (presetId, programId, controlId),
                   FOREIGN KEY (presetId) REFERENCES presets (presetId));
