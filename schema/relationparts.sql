CREATE TABLE "relationparts" (
  "relationid" INTEGER PRIMARY KEY NOT NULL,
  "othertype" TEXT NOT NULL,
  "otherid" INTEGER NOT NULL,
   UNIQUE ("relationid","otherid") ON CONFLICT IGNORE
);
