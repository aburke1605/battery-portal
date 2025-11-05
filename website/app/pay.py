# server.py
from flask import Flask, Blueprint, jsonify, request
import stripe
import os

pay = Blueprint("pay", __name__, url_prefix="/pay")

stripe.api_key = os.getenv("STRIPE_SECRET_KEY")


@pay.route("/create-payment-intent", methods=["POST"])
def create_payment_intent():
    data = request.get_json()
    try:
        intent = stripe.PaymentIntent.create(
            amount=int(data["amount"] * 100),
            currency="usd",
            automatic_payment_methods={"enabled": True},
        )
        return jsonify({"clientSecret": intent.client_secret})
    except stripe.error.StripeError as e:
        return jsonify(error=str(e)), 400
