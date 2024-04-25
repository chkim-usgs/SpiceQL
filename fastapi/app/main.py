"""Module providing SpiceQL endpoints"""

from ast import literal_eval
from typing import Annotated, Any
from fastapi import FastAPI, Query
from pydantic import BaseModel, Field
from starlette.responses import RedirectResponse
import pyspiceql

SEARCH_KERNELS_BOOL = True

# Models
class MessageItem(BaseModel):
    message: str

class ResultModel(BaseModel):
    result: Any = Field(serialization_alias='return')

class ErrorModel(BaseModel):
    error: str

class ResponseModel(BaseModel):
    statusCode: int
    body: ResultModel | ErrorModel

# Create FastAPI instance
app = FastAPI()

# General endpoints
@app.get("/", include_in_schema=False)
async def root():
    return RedirectResponse(url="/docs")

@app.post("/customMessage")
async def message(
    message_item: MessageItem
    ):
    return {"message": message_item.message}


# SpiceQL endpoints
@app.get("/getTargetStates")
async def getTargetStates(
    ets: Annotated[list[float], Query()] | str,
    target: str,
    observer: str,
    frame: str,
    abcorr: str,
    mission: str,
    ckQuality: str = "",
    spkQuality: str = ""):
    try:
        if isinstance(ets, str):
            ets = literal_eval(ets)
        result = pyspiceql.getTargetStates(ets, target, observer, frame, abcorr, mission, ckQuality, spkQuality, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)
    
@app.get("/getTargetOrientations")
async def getTargetOrientations(
    ets: Annotated[list[float], Query()] | str,
    toFrame: int,
    refFrame: int,
    mission: str,
    ckQuality: str = ""):
    try:
        if isinstance(ets, str):
            ets = literal_eval(ets)
        result = pyspiceql.getTargetOrientations(ets, toFrame, refFrame, mission, ckQuality, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)  
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/strSclkToEt")
async def strSclkToEt(
    frameCode: int,
    sclk: str,
    mission: str):
    try:
        result = pyspiceql.strSclkToEt(frameCode, sclk, mission, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/doubleSclkToEt")
async def doubleSclkToEt(
    frameCode: int,
    sclk: float,
    mission: str):
    try:
        result = pyspiceql.doubleSclkToEt(frameCode, sclk, mission, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/utcToEt")
async def utcToEt(
    utc: str):
    try:
        result = pyspiceql.utcToEt(utc, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/translateNameToCode")
async def translateNameToCode(
    frame: str,
    mission: str):
    try:
        result = pyspiceql.translateNameToCode(frame, mission, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/translateCodeToName")
async def translateCodeToName(
    frame: int,
    mission: str):
    try:
        result = pyspiceql.translateCodeToName(frame, mission, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/getFrameInfo")
async def getFrameInfo(
    frame: int,
    mission: str):
    try:
        result = pyspiceql.getFrameInfo(frame, mission, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/getTargetFrameInfo")
async def getTargetFrameInfo(
    targetId: int,
    mission: str):
    try:
        result = pyspiceql.getTargetFrameInfo(targetId, mission, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/findMissionKeywords")
async def findMissionKeywords(
    key: str,
    mission: str):
    try:
        result = pyspiceql.findMissionKeywords(key, mission, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/findTargetKeywords")
async def findTargetKeywords(
    key: str,
    mission: str):
    try:
        result = pyspiceql.findTargetKeywords(key, mission, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)

@app.get("/frameTrace")
async def frameTrace(
    et: float,
    initialFrame: int,
    mission: str,
    ckQuality: str = ""):
    try:
        result = pyspiceql.frameTrace(et, initialFrame, mission, ckQuality, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)
    
@app.get("/extractExactCkTimes")
async def extractExactCkTimes(
    observStart: float,
    observEnd: float,
    targetFrame: int,
    mission: str,
    ckQuality: str = ""):
    try:
        result = pyspiceql.extractExactCkTimes(observStart, observEnd, targetFrame, mission, ckQuality, SEARCH_KERNELS_BOOL)
        body = ResultModel(result=result)
        return ResponseModel(statusCode=200, body=body)
    except Exception as e:
        body = ErrorModel(error=str(e))
        return ResponseModel(statusCode=500, body=body)
    