CREATE TABLE "relationparts" (
  "relationid" TEXT NOT NULL,
  "othertype" TEXT NOT NULL,
  "otherid" TEXT NOT NULL,
   UNIQUE ("relationid","otherid") ON CONFLICT IGNORE
);
